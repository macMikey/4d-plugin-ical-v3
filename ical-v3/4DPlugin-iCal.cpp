/* --------------------------------------------------------------------------------
 #
 #  4DPlugin-iCal.cpp
 #	source generated by 4D Plugin Wizard
 #	Project : iCal
 #	author : miyako
 #	2020/03/17
 #  
 # --------------------------------------------------------------------------------*/

#include "4DPlugin-iCal.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#pragma mark permission

std::mutex mutex_permission;

request_permission_t granted_permission = request_permission_unknown;

BOOL check_permission(PA_ObjectRef status) {
    
    BOOL returnValue = false;
    
    if(status) {
        
        switch (granted_permission) {
                
            case request_permission_authorized:
                returnValue = true;
                ob_set_b(status, L"success", returnValue);
                break;
                
            case request_permission_denied:
                returnValue = false;
                ob_set_b(status, L"success", returnValue);
                ob_set_s(status, L"errorMessage", "permission denied");
                return false;
                break;
                
            case request_permission_restricted:
                returnValue = false;
                ob_set_b(status, L"success", returnValue);
                ob_set_s(status, L"errorMessage", "permission restricted");
                break;
                
            case request_permission_not_determined:
                returnValue = false;
                ob_set_b(status, L"success", returnValue);
                ob_set_s(status, L"errorMessage", "permission not determined");
                break;
                
            default:
                break;
        }
    }
    
    return returnValue;
}

request_permission_t requestPermission(void) {
    
    std::lock_guard<std::mutex> lock(mutex_permission);
    
    if (@available(macOS 10.9, *)) {
        
        switch ([EKEventStore authorizationStatusForEntityType:EKEntityTypeEvent])
        {
            case EKAuthorizationStatusNotDetermined:
            {
                EKEventStore *store = [EKEventStore new];
                [store requestAccessToEntityType:EKEntityTypeEvent completion:^(BOOL granted, NSError * _Nullable error) {
                    if (granted) {
                        granted_permission = request_permission_authorized;
                    }
                }];
                granted_permission = request_permission_not_determined;
                break;
            }
            break;
                
            case EKAuthorizationStatusRestricted:
                granted_permission = request_permission_restricted;
                break;
                
            case EKAuthorizationStatusDenied:
                granted_permission = request_permission_denied;
                break;
                
            case EKAuthorizationStatusAuthorized:
                granted_permission = request_permission_authorized;
                break;
        }

    }
    
    return granted_permission;
}

void iCal_Request_permisson(PA_PluginParameters params) {

    PA_ObjectRef status = PA_CreateObject();
    
    NSBundle *mainBundle = [NSBundle mainBundle];
    if(mainBundle) {
        NSDictionary *infoDictionary = [mainBundle infoDictionary];
        if(infoDictionary) {
            NSString *calendarUsageDescription = [infoDictionary objectForKey:@"NSCalendarsUsageDescription"];
            if(calendarUsageDescription) {
             
                SecTaskRef sec = SecTaskCreateFromSelf(kCFAllocatorMalloc);
                CFErrorRef err = nil;
                CFBooleanRef boolValue = (CFBooleanRef)SecTaskCopyValueForEntitlement(sec,
                                                                                      CFSTR("com.apple.security.personal-information.calendars"),
                                                                                      &err);
                if((!err) && (boolValue)){
                    if(boolValue) {
                        if(CFBooleanGetValue(boolValue)) {
                             
                            PA_RunInMainProcess((PA_RunInMainProcessProcPtr)requestPermission, NULL);
                            
                            check_permission(status);
 
                        }else{
                            ob_set_b(status, L"success", false);
                            ob_set_s(status, L"errorMessage", "com.apple.security.personal-information.calendars is set to false in app entitlement");
                        }
                        CFRelease(boolValue);
                    }
                }else{
                    ob_set_b(status, L"success", false);
                    ob_set_s(status, L"errorMessage", "com.apple.security.personal-information.calendars is missing in app entitlement");
                }
                CFRelease(sec);
            }else{
                ob_set_b(status, L"success", false);
                ob_set_s(status, L"errorMessage", "NScalendarUsageDescription is missing in app info.plist");
            }
        }else{
            ob_set_b(status, L"success", false);
            ob_set_s(status, L"errorMessage", "failed to locate [mainBundle infoDictionary]");
        }
    }else{
        ob_set_b(status, L"success", false);
        ob_set_s(status, L"errorMessage", "failed to locate [NSBundle mainBundle]");
    }
    
    PA_ReturnObject(params, status);
}

#pragma mark -

void PluginMain(PA_long32 selector, PA_PluginParameters params) {
    
	try
	{
        switch(selector)
        {
			// --- iCal
            
			case 1 :
				iCal_Request_permisson(params);
				break;
            case 2 :
                iCal_QUERY_EVENT(params);
                break;
            case 3 :
                iCal_GET_CALENDAR_LIST(params);
                break;
                
        }

	}
	catch(...)
	{

	}
}

NSArray *ob_get_calendars(PA_ObjectRef options, CalCalendarStore *calendarStore) {
    
    NSMutableArray *value = nil;
    
    if(options){
        if(calendarStore){
            PA_CollectionRef calendars = ob_get_c(options, L"calendars");
            if(calendars){
                value = [[NSMutableArray alloc]init];
                for(PA_long32 i = 0; i < PA_GetCollectionLength(calendars); ++i){
                    PA_Variable v = PA_GetCollectionElement(calendars, i);
                    if(PA_GetVariableKind(v) == eVK_Object){
                        PA_ObjectRef o = PA_GetObjectVariable(v);
                        if(o){
                            NSString *uid = ob_get_v(o, L"uid");
                            if(uid){
                                CalCalendar *calendar = [calendarStore calendarWithUID:uid];
                                [value addObject:calendar];
                            }
                        }
                    }
                }
            }
        }
    }
    return value;
}

void iCal_QUERY_EVENT(PA_PluginParameters params) {
    
    PA_ObjectRef status = PA_CreateObject();
    
    if(check_permission(status)) {
        CalCalendarStore *defaultCalendarStore = [CalCalendarStore defaultCalendarStore];
        if(defaultCalendarStore) {
            
            PA_CollectionRef _events = PA_CreateCollection();
            
            PA_ObjectRef options = PA_GetObjectParameter(params, 1);
            if(options){
                
                NSDate *startDate = ob_get_d(options, L"startDate");
                NSDate *endDate = ob_get_d(options, L"endDate");
                
                if(startDate){
                    if(endDate){
                        NSArray *calendars = ob_get_calendars(options, defaultCalendarStore);
                        
                        if(calendars){
                            
                            NSPredicate *predicate = [CalCalendarStore eventPredicateWithStartDate:startDate
                                                                                           endDate:endDate
                                                                                         calendars:calendars];
                            if(predicate){
                                NSArray *events = [defaultCalendarStore eventsWithPredicate:predicate];
                                
                                time_t startTime = time(0);
                                
                                for(unsigned int i = 0; i < [events count]; ++i) {
                                    
                                    time_t now = time(0);
                                    time_t elapsedTime = abs(startTime - now);
                                    if(elapsedTime > 0)
                                    {
                                        startTime = now;
                                        PA_YieldAbsolute();
                                    }
                                    if([[events objectAtIndex:i]isMemberOfClass:[CalEvent class]]) {
                                        
                                        CalEvent *event = [events objectAtIndex:i];
                                        
                                        PA_ObjectRef _event = PA_CreateObject();
                                        
                                        ob_set_v(_event, L"uid", event.uid);
                                        ob_set_v(_event, L"location", event.location);
                                        ob_set_v(_event, L"notes", event.notes);
                                        ob_set_v(_event, L"title", event.title);
                                        
                                        ob_set_u(_event, L"url", event.url);
                                        
                                        ob_set_d(_event, L"endDate", event.endDate);
                                        ob_set_d(_event, L"occurrence", event.occurrence);
                                        ob_set_d(_event, L"startDate", event.startDate);
                                        ob_set_d(_event, L"dateStamp", event.dateStamp);
                                        
                                        ob_set_b(_event, L"isAllDay", event.isAllDay);
                                        ob_set_b(_event, L"isDetached", event.isDetached);
                                        
                                        PA_Variable v = PA_CreateVariable(eVK_Object);
                                        PA_SetObjectVariable(&v, _event);
                                        PA_SetCollectionElement(_events, PA_GetCollectionLength(_events), v);
                                        PA_ClearVariable(&v);
                                    }
                                }
                                ob_set_c(status, L"events", _events);
                            }else{
                                ob_set_b(status, L"success", false);
                                ob_set_s(status, L"errorMessage", "invalid predicate");
                            }
                        }else{
                            ob_set_b(status, L"success", false);
                            ob_set_s(status, L"errorMessage", "invalid calendars");
                        }
                    }else{
                        ob_set_b(status, L"success", false);
                        ob_set_s(status, L"errorMessage", "invalid endDate");
                    }
                }else{
                    ob_set_b(status, L"success", false);
                    ob_set_s(status, L"errorMessage", "invalid startDate");
                }
            }
        }
    }
    PA_ReturnObject(params, status);
}

unsigned int getColorRGB(NSColor *color) {
    
    unsigned int rgb = 0;
    
    if(color)
    {
        color = [color colorUsingColorSpace:[NSColorSpace displayP3ColorSpace]];
        
        /*
         color = [color colorUsingColorSpace:[NSColorSpace deviceRGBColorSpace]];//NSDeviceRGBColorSpace
         color = [color colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
         color = [color colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];//NSCalibratedRGBColorSpace
         */
        
        CGFloat red, green, blue, alpha;
        [color getRed:&red green:&green blue:&blue alpha:&alpha];
        
        rgb +=
        
        /*
         +((unsigned int)(red      * 255.99999f) << 16)
         +((unsigned int)(green    * 255.99999f) << 8)
         + (unsigned int)(blue     * 255.99999f);
         */
        
        +((unsigned int)floor((CGFloat)(red      * 0xFF) + 0.5f) << 16)
        +((unsigned int)floor((CGFloat)(green    * 0xFF) + 0.5f) << 8)
        + (unsigned int)floor((CGFloat)(blue     * 0xFF) + 0.5f);
    }
    
    return rgb;
}

void iCal_GET_CALENDAR_LIST(PA_PluginParameters params) {
    
    PA_ObjectRef status = PA_CreateObject();
    
    if(check_permission(status)) {
        CalCalendarStore *defaultCalendarStore = [CalCalendarStore defaultCalendarStore];
        if(defaultCalendarStore) {
            
            PA_CollectionRef _calendars = PA_CreateCollection();
            
            NSArray *calendars = [defaultCalendarStore calendars];
            for(unsigned int i = 0; i < [calendars count]; ++i) {

                if([[calendars objectAtIndex:i]isMemberOfClass:[CalCalendar class]]) {
                    
                    CalCalendar *calendar = [calendars objectAtIndex:i];

                    PA_ObjectRef _calendar = PA_CreateObject();

                    ob_set_v(_calendar, L"title", calendar.title);
                    ob_set_v(_calendar, L"uid", calendar.uid);
                    ob_set_v(_calendar, L"notes", calendar.notes);
                    ob_set_v(_calendar, L"type", calendar.type);

                    ob_set_b(_calendar, L"isEditable", calendar.isEditable);
                    ob_set_n(_calendar, L"color", calendar.color ? getColorRGB(calendar.color) : 0L);

                    PA_Variable v = PA_CreateVariable(eVK_Object);
                    PA_SetObjectVariable(&v, _calendar);
                    PA_SetCollectionElement(_calendars, PA_GetCollectionLength(_calendars), v);
                    PA_ClearVariable(&v);
 
                }
                
            }//[calendars count]
            
            ob_set_c(status, L"calendars", _calendars);
            
        }//defaultCalendarStore
    }
    
    PA_ReturnObject(params, status);
}

#pragma GCC diagnostic pop

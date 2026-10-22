#include    <sega_xpt.h>
#include    "per_x.h"
#include    "pad.h"

volatile Uint16    PadData1  = 0x0000;
volatile Uint16    PadData1E = 0x0000;
volatile Uint16    PadData2  = 0x0000;
volatile Uint16    PadData2E = 0x0000;

static SysPort    *__port = NULL;

void padInit(void) {
    __port = PER_OpenPort();
}

void padRead(void) {
    if( __port != NULL ){
        const SysDevice    *device;
        PER_GetPort( __port );

        if(( device = PER_GetDeviceR( &__port[0], 0 )) != NULL ){
            trigger_t    prev = PadData1;
            trigger_t    current = PER_GetTrigger( device );

            PadData1  = current;
            PadData1E = PER_GetPressEdge( prev, current );
        }
        else{
            PadData1 = PadData1E = 0;
        }

        if(( device = PER_GetDeviceR( &__port[1], 0 )) != NULL ){
            trigger_t    prev = PadData2;
            trigger_t    current = PER_GetTrigger( device );

            PadData2  = current;
            PadData2E = PER_GetPressEdge( prev, current );
        }
        else{
            PadData2 = PadData2E = 0;
        }
    }
}

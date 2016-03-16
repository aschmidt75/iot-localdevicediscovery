
#define SERIAL_DEBUG

UDP udp;

// Structure of discovery_buffer:
// 0..3     local IP address, 4 octets
// 4..7     netmask, 4 octets
// 8..11    gateway IP, 4 octets
// 12..15   version info, 4 bytes
// 16..16+25    device ID, including \0 byte
const size_t    discovery_bufferSize = 64; 
unsigned char   discovery_buffer[discovery_bufferSize];

IPAddress       discovery_remoteIP(239,1,10,10);
int             discovery_remotePort = 9000;

#define DISCOVERY_STATE_NEW             0
#define DISCOVERY_STATE_SENDING_START   1
#define DISCOVERY_STATE_SENDING_ERROR   2
#define DISCOVERY_STATE_SENDING_OK      3
#define DISCOVERY_STATE_DONE            4

// Maximum number of successful discovery messages sent.
// After this count has been reached, discovery messages
// are discontinued.
#define DISCOVERY_SEND_MAX_COUNT        5

int             discovery_state = DISCOVERY_STATE_NEW;
int             discovery_send_count = 0;
Timer           *discovery_timer = NULL;

/**
 * cp_ipa
 * copy the four octets of an IPAddress to an array
 */
void discovery_cp_ipa(unsigned char *dest, IPAddress addr) {
    dest[0] = addr[0];
    dest[1] = addr[1];
    dest[2] = addr[2];
    dest[3] = addr[3];
}

/**
 * set_buffer_wifidata
 * copies the local IP, subnet mask and gateway IP to global buffer var
 */
void discovery_set_buffer_wifidata() {
    discovery_cp_ipa(&discovery_buffer[0],WiFi.localIP());
    discovery_cp_ipa(&discovery_buffer[4],WiFi.subnetMask());
    discovery_cp_ipa(&discovery_buffer[8],WiFi.gatewayIP());
    unsigned long v = System.versionNumber();
    discovery_buffer[12] = (unsigned char)((v >> 24) & 0xff);
    discovery_buffer[13] = (unsigned char)((v >> 16) & 0xff);
    discovery_buffer[14] = (unsigned char)((v >> 8) & 0xff);
    discovery_buffer[15] = (unsigned char)(v & 0xff);
    
    System.deviceID().toCharArray(( char*)(&discovery_buffer[16]), 25);
}


void discovery_information_thread() {
    #ifdef SERIAL_DEBUG
    Serial.printlnf("send_discovery_information_thread(state=%d)", discovery_state);
    #endif
    
    if (discovery_state == DISCOVERY_STATE_DONE) {
        #ifdef SERIAL_DEBUG
        Serial.println("send_discovery_information_thread: done sending, disposing timer");
        #endif
        if ( discovery_timer != NULL) {
            discovery_timer->dispose();
        }
        return;
    }
    if (discovery_state == DISCOVERY_STATE_NEW) {
        discovery_set_buffer_wifidata();
        discovery_state = DISCOVERY_STATE_SENDING_START;
        #ifdef SERIAL_DEBUG
        Serial.printlnf("send_discovery_information_thread: starting to send to %d.%d.%d.%d:%d", 
            discovery_remoteIP[0],discovery_remoteIP[1],discovery_remoteIP[2],discovery_remoteIP[3],discovery_remotePort);
        #endif
       
        return;
    }
    if (
        (discovery_state == DISCOVERY_STATE_SENDING_START) ||
        (discovery_state == DISCOVERY_STATE_SENDING_OK) ||
        (discovery_state == DISCOVERY_STATE_SENDING_ERROR) 
    ) {
        if (discovery_send_count >= DISCOVERY_SEND_MAX_COUNT) {
            discovery_state = DISCOVERY_STATE_DONE;
            #ifdef SERIAL_DEBUG
            Serial.println("send_discovery_information_thread: exceeded maximum send count.");
            #endif
            discovery_timer->changePeriod(100);
            return;
        }

        int res = udp.sendPacket(discovery_buffer, discovery_bufferSize, 
            discovery_remoteIP, discovery_remotePort);
            
        if (res > 0) {
            discovery_state = DISCOVERY_STATE_SENDING_OK;
            discovery_send_count++;
            // go a bit slower now
            discovery_timer->changePeriod(1000*discovery_send_count);
            #ifdef SERIAL_DEBUG
            Serial.printlnf("send_discovery_information_thread: successfully sent, count=%d", discovery_send_count);
            #endif
            return;
        } else {
            discovery_state = DISCOVERY_STATE_SENDING_ERROR;
            udp.begin(0);
            // go a bit faster now
            discovery_timer->changePeriod(100);
            #ifdef SERIAL_DEBUG
            Serial.printlnf("send_discovery_information_thread: error=%d, retrying.", res);
            #endif
            return;
            
        }
    }
    
}

// Create a timer, so that our discovery routine gets
// called every 2 secs.
Timer timer(2000,discovery_information_thread);

void setup() {
#ifdef SERIAL_DEBUG
    Serial.begin(9600);
#endif

    udp.begin(0);
   
    // initialize our discovery code ,start the timer
    discovery_state = DISCOVERY_STATE_NEW;
    discovery_timer = &timer;
    timer.start();
    
}

void loop() {
    // your code goes here
}

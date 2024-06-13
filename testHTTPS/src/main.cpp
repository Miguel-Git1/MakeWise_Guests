#include "Arduino.h"
#include <WiFi.h>
#include <WiFiClient.h> 
#include <ESP32_FTPClient.h>
#include "soc/soc.h"
#include <esp_camera.h>
#include "soc/rtc_cntl_reg.h"
#include <SPIFFS.h>
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <libssh2_setup.h>


// Needed
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>

#ifdef WIN32
#define write(f, b, c)  write((f), (b), (unsigned int)(c))
#endif
 
 
#include <stdio.h>
#include <string.h>


// For the WiFi
#define WIFI_SSID "WI-7-2.4"
#define WIFI_PASS "inov84ever"


// For the Camera
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22


char ftp_server[] = "54.73.73.1";
char ftp_user[]   = "fD50i3hPckJtPGCS7ihABeuk3bVjDYcM";
char ftp_pass[]   = "9RZtqX1MdYqAw3uh";
char working_dir[]   = ".";


// you can pass a FTP timeout and debbug mode on the last 2 arguments
ESP32_FTPClient ftp(ftp_server, 2221, ftp_user, ftp_pass, 5000, 2);

static const char *pubkey = "/home/username/.ssh/id_rsa.pub";
static const char *privkey = "/home/username/.ssh/id_rsa";
static const char *username = "username";
static const char *password = "password";
static const char *sftppath = "/tmp/TEST";

void setupCamera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_SVGA;   // The frame will be 800x600bl
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  if (!SPIFFS.begin(true))
  {
    Serial.println("\nSPIFFS initialisation failed!\n");
  }
  else
  {
    Serial.printf("\nDone with SPIFFS!\n");
  }
  for (int i = 0; i < 5; i++)
  {
    camera_fb_t *fb = NULL;
    fb = esp_camera_fb_get();

    if (!fb)
    {
      esp_camera_fb_return(fb);
      fb = NULL;
      Serial.printf("\n--- ERROR: Camera capture failed");
    }
    else
    {
      esp_camera_fb_return(fb);
      Serial.println("\n--- Good Image ---");
    }
  }
}


static void kbd_callback(const char *name, int name_len,
                         const char *instruction, int instruction_len,
                         int num_prompts,
                         const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                         LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
                         void **abstract)
{
    int i;
    size_t n;
    char buf[1024];
    (void)abstract;
 
    fprintf(stderr, "Performing keyboard-interactive authentication.\n");
 
    fprintf(stderr, "Authentication name: '");
    fwrite(name, 1, name_len, stderr);
    fprintf(stderr, "'\n");
 
    fprintf(stderr, "Authentication instruction: '");
    fwrite(instruction, 1, instruction_len, stderr);
    fprintf(stderr, "'\n");
 
    fprintf(stderr, "Number of prompts: %d\n\n", num_prompts);
 
    for(i = 0; i < num_prompts; i++) {
        fprintf(stderr, "Prompt %d from server: '", i);
        fwrite(prompts[i].text, 1, prompts[i].length, stderr);
        fprintf(stderr, "'\n");
 
        fprintf(stderr, "Please type response: ");
        fgets(buf, sizeof(buf), stdin);
        n = strlen(buf);
        while(n > 0 && strchr("\r\n", buf[n - 1]))
            n--;
        buf[n] = 0;
 
        responses[i].text = strdup(buf);
        responses[i].length = (unsigned int)n;
 
        fprintf(stderr, "Response %d from user is '", i);
        fwrite(responses[i].text, 1, responses[i].length, stderr);
        fprintf(stderr, "'\n\n");
    }
 
    fprintf(stderr,
        "Done. Sending keyboard-interactive responses to server now.\n");
}



void setup()
{
  Serial.begin( 115200 );
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  setupCamera();
  delay(5000);
  WiFi.begin( WIFI_SSID, WIFI_PASS );
  Serial.println("Connecting Wifi...");
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");
  }

  char *argv[] {};
  int argc;
  uint32_t hostaddr;
  libssh2_socket_t sock;
  int i, auth_pw = 0;
  struct sockaddr_in sin;
  const char *fingerprint;
  char *userauthlist;
  int rc;
  LIBSSH2_SESSION *session = NULL;
  LIBSSH2_SFTP *sftp_session;
  LIBSSH2_SFTP_HANDLE *sftp_handle;

  #ifdef WIN32
    
    WSADATA wsadata;
 
    rc = WSAStartup(MAKEWORD(2, 0), &wsadata);
    if(rc) {
        fprintf(stderr, "WSAStartup failed with error: %d\n", rc);
        return 1;
    }
  #endif
  
  
  if(argc > 1) {
    hostaddr = inet_addr(argv[1]);
  }
  else {
    hostaddr = htonl(0x7F000001);
  }
  if(argc > 2) {
    username = argv[2];
  }
  if(argc > 3) {
    password = argv[3];
  }
  if(argc > 4) {
    sftppath = argv[4];
  }

  rc = libssh2_init(0);
  if(rc) {
    fprintf(stderr, "libssh2 initialization failed (%d)\n", rc);
    return;
  }
 
    /*
     * The application code is responsible for creating the socket
     * and establishing the connection
     */ 
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock == LIBSSH2_INVALID_SOCKET) {
    fprintf(stderr, "failed to create socket.\n");
    goto shutdown;
  }
 
  sin.sin_family = AF_INET;
  sin.sin_port = htons(22);
  sin.sin_addr.s_addr = hostaddr;
  if(connect(sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in))) {
    fprintf(stderr, "failed to connect.\n");
    goto shutdown;
  }
 
    /* Create a session instance */ 
  session = libssh2_session_init();

  if(!session) {
    fprintf(stderr, "Could not initialize SSH session.\n");
    goto shutdown;
  }
 
    /* Since we have set non-blocking, tell libssh2 we are blocking */ 
  libssh2_session_set_blocking(session, 1);

 
    /* ... start it up. This will trade welcome banners, exchange keys,
     * and setup crypto, compression, and MAC layers
     */ 
  rc = libssh2_session_handshake(session, sock);

  if(rc) {
    fprintf(stderr, "Failure establishing SSH session: %d\n", rc);
    goto shutdown;
  }
 
    /* At this point we have not yet authenticated.  The first thing to do
     * is check the hostkey's fingerprint against our known hosts Your app
     * may have it hard coded, may go to a file, may present it to the
     * user, that's your call
     */ 
  fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);

  fprintf(stderr, "Fingerprint: ");
  for(i = 0; i < 20; i++) {
    fprintf(stderr, "%02X ", (unsigned char)fingerprint[i]);
  }
  fprintf(stderr, "\n");
 
    /* check what authentication methods are available */ 
  userauthlist = libssh2_userauth_list(session, username, (unsigned int)strlen(username));
    
  if(userauthlist) {
    fprintf(stderr, "Authentication methods: %s\n", userauthlist);
    if(strstr(userauthlist, "password")) {
        auth_pw |= 1;
    }
    if(strstr(userauthlist, "keyboard-interactive")) {
        auth_pw |= 2;
    }
    if(strstr(userauthlist, "publickey")) {
        auth_pw |= 4;
    }
 
        /* check for options */ 
    if(argc > 5) {
            if((auth_pw & 1) && !strcmp(argv[5], "-p")) {
                auth_pw = 1;
            }
            if((auth_pw & 2) && !strcmp(argv[5], "-i")) {
                auth_pw = 2;
            }
            if((auth_pw & 4) && !strcmp(argv[5], "-k")) {
                auth_pw = 4;
            }
        }
 
        if(auth_pw & 1) {
            /* We could authenticate via password */ 
            if(libssh2_userauth_password(session, username, password)) {

                fprintf(stderr, "Authentication by password failed.\n");
                goto shutdown;
            }
        }
        else if(auth_pw & 2) {
            /* Or via keyboard-interactive */ 
            if(libssh2_userauth_keyboard_interactive(session, username,

                                                     &kbd_callback) ) {
                fprintf(stderr,
                        "Authentication by keyboard-interactive failed.\n");
                goto shutdown;
            }
            else {
                fprintf(stderr,
                        "Authentication by keyboard-interactive succeeded.\n");
            }
        }
        else if(auth_pw & 4) {
            /* Or by public key */ 
            if(libssh2_userauth_publickey_fromfile(session, username,

                                                   pubkey, privkey,
                                                   password)) {
                fprintf(stderr, "Authentication by public key failed.\n");
                goto shutdown;
            }
            else {
                fprintf(stderr, "Authentication by public key succeeded.\n");
            }
        }
        else {
            fprintf(stderr, "No supported authentication methods found.\n");
            goto shutdown;
        }
    }
 
    fprintf(stderr, "libssh2_sftp_init().\n");

    sftp_session = libssh2_sftp_init(session);

 
    if(!sftp_session) {
        fprintf(stderr, "Unable to init SFTP session\n");
        goto shutdown;
    }
 
    fprintf(stderr, "libssh2_sftp_open().\n");

    /* Request a file via SFTP */ 
    sftp_handle = libssh2_sftp_open(sftp_session, sftppath,

                                    LIBSSH2_FXF_READ, 0);
    if(!sftp_handle) {
        fprintf(stderr, "Unable to open file with SFTP: %ld\n",
                libssh2_sftp_last_error(sftp_session));

        goto shutdown;
    }
 
    fprintf(stderr, "libssh2_sftp_open() is done, now receive data.\n");

    do {
        char mem[1024];
        ssize_t nread;
 
        /* loop until we fail */ 
        fprintf(stderr, "libssh2_sftp_read().\n");

        nread = libssh2_sftp_read(sftp_handle, mem, sizeof(mem));

        if(nread > 0) {
            write(1, mem, nread);
        }
        else {
            break;
        }
    } while(1);
 
    libssh2_sftp_close(sftp_handle);

    libssh2_sftp_shutdown(sftp_session);

 
shutdown:
 
    if(session) {
        libssh2_session_disconnect(session, "Normal Shutdown");

        libssh2_session_free(session);

    }
 
    if(sock != LIBSSH2_INVALID_SOCKET) {
        shutdown(sock, 2);
#ifdef WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }
 
    fprintf(stderr, "all done\n");
 
    libssh2_exit();
}

void loop()
{

}
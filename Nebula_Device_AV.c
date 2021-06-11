#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fcntl.h>

#include "Nebula_Device_AV.h"
#include "sample_constants.h"


//########################################################
//# Nebula configurations data
//########################################################
static char gUdid[MAX_UDID_LENGTH + 1] ={0};
static char gPinCode[MAX_PIN_CODE_LENGTH + 1] = {0};
static char gSecretId[MAX_NEBULA_SECRETID_LENGTH + 1] = {0};

static unsigned int gBindAbort = 0;
static struct timeval gNoSessionTime = {0};
static unsigned int gBindType = SERVER_BIND;
static char gProfilePath[128] = {0};
static NebulaJsonObject *gProfileJsonObj = NULL;
static VSaaSContractInfo gVsaasContractInfo = {0};
static bool gEnableVsaas = false;
static bool gVsaasConfigExist = false;
static bool gEnablePushNotification = false;
static bool gSettingsExist = false;
static bool gEnableWakeUp = false;
static bool gProgressRun = false;

//########################################################
//# AV file path
//########################################################
static char gLiveIframePath[] = "./video/I_000.bin";
static char gAudioFilePath[] = "./audio/beethoven_8k_16bit_mono.raw";
static char gVsaasInfoFilePath[] = "./vsaasinfo";
static char gRecordFile[] = "frames";


AV_Client gClientInfo[MAX_CLIENT_NUMBER];

static int gOnlineNum = 0;

//########################################################
//# setting file path
//########################################################
static char gDefaultSettingsFilePath[] = "./device_settings.txt";

//########################################################
//# identity file path
//########################################################
static char gUserIdentitiesFilePath[] = "./identities_list.txt";

//########################################################
//# Get Timestamp
//########################################################
unsigned int GetTimeStampMs()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

unsigned int GetTimeStampSec()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec);
}

//########################################################
//# Print error message
//########################################################
static void PrintErrHandling(int error)
{
    switch (error)
    {
    case IOTC_ER_MASTER_NOT_RESPONSE:
        //-60 IOTC_ER_MASTER_NOT_RESPONSE
        printf("[Error code : %d]\n", IOTC_ER_MASTER_NOT_RESPONSE);
        printf("Master server doesn't respond.\n");
        printf("Please check the network wheather it could connect to the Internet.\n");
        break;
    case IOTC_ER_SERVER_NOT_RESPONSE:
        //-1 IOTC_ER_SERVER_NOT_RESPONSE
        printf("[Error code : %d]\n", IOTC_ER_SERVER_NOT_RESPONSE);
        printf("P2P Server doesn't respond.\n");
        printf("Please check the network wheather it could connect to the Internet.\n");
        break;
    case IOTC_ER_FAIL_RESOLVE_HOSTNAME:
        //-2 IOTC_ER_FAIL_RESOLVE_HOSTNAME
        printf("[Error code : %d]\n", IOTC_ER_FAIL_RESOLVE_HOSTNAME);
        printf("Can't resolve hostname.\n");
        break;
    case IOTC_ER_ALREADY_INITIALIZED:
        //-3 IOTC_ER_ALREADY_INITIALIZED
        printf("[Error code : %d]\n", IOTC_ER_ALREADY_INITIALIZED);
        printf("Already initialized.\n");
        break;
    case IOTC_ER_FAIL_CREATE_MUTEX:
        //-4 IOTC_ER_FAIL_CREATE_MUTEX
        printf("[Error code : %d]\n", IOTC_ER_FAIL_CREATE_MUTEX);
        printf("Can't create mutex.\n");
        break;
    case IOTC_ER_FAIL_CREATE_THREAD:
        //-5 IOTC_ER_FAIL_CREATE_THREAD
        printf("[Error code : %d]\n", IOTC_ER_FAIL_CREATE_THREAD);
        printf("Can't create thread.\n");
        break;
    case IOTC_ER_UNLICENSE:
        //-10 IOTC_ER_UNLICENSE
        printf("[Error code : %d]\n", IOTC_ER_UNLICENSE);
        printf("This UID is unlicense.\n");
        printf("Check your UID.\n");
        break;
    case IOTC_ER_NOT_INITIALIZED:
        //-12 IOTC_ER_NOT_INITIALIZED
        printf("[Error code : %d]\n", IOTC_ER_NOT_INITIALIZED);
        printf("Please initialize the IOTCAPI first.\n");
        break;
    case IOTC_ER_TIMEOUT:
        //-13 IOTC_ER_TIMEOUT
        break;
    case IOTC_ER_INVALID_SID:
        //-14 IOTC_ER_INVALID_SID
        printf("[Error code : %d]\n", IOTC_ER_INVALID_SID);
        printf("This SID is invalid.\n");
        printf("Please check it again.\n");
        break;
    case IOTC_ER_EXCEED_MAX_SESSION:
        //-18 IOTC_ER_EXCEED_MAX_SESSION
        printf("[Error code : %d]\n", IOTC_ER_EXCEED_MAX_SESSION);
        printf("[Warning]\n");
        printf("The amount of session reach to the maximum.\n");
        printf("It cannot be connected unless the session is released.\n");
        break;
    case IOTC_ER_CAN_NOT_FIND_DEVICE:
        //-19 IOTC_ER_CAN_NOT_FIND_DEVICE
        printf("[Error code : %d]\n", IOTC_ER_CAN_NOT_FIND_DEVICE);
        printf("Device didn't register on server, so we can't find device.\n");
        printf("Please check the device again.\n");
        printf("Retry...\n");
        break;
    case IOTC_ER_SESSION_CLOSE_BY_REMOTE:
        //-22 IOTC_ER_SESSION_CLOSE_BY_REMOTE
        printf("[Error code : %d]\n", IOTC_ER_SESSION_CLOSE_BY_REMOTE);
        printf("Session is closed by remote so we can't access.\n");
        printf("Please close it or establish session again.\n");
        break;
    case IOTC_ER_REMOTE_TIMEOUT_DISCONNECT:
        //-23 IOTC_ER_REMOTE_TIMEOUT_DISCONNECT
        printf("[Error code : %d]\n", IOTC_ER_REMOTE_TIMEOUT_DISCONNECT);
        printf("We can't receive an acknowledgement character within a TIMEOUT.\n");
        printf("It might that the session is disconnected by remote.\n");
        printf("Please check the network wheather it is busy or not.\n");
        printf("And check the device and user equipment work well.\n");
        break;
    case IOTC_ER_DEVICE_NOT_LISTENING:
        //-24 IOTC_ER_DEVICE_NOT_LISTENING
        printf("[Error code : %d]\n", IOTC_ER_DEVICE_NOT_LISTENING);
        printf("Device doesn't listen or the sessions of device reach to maximum.\n");
        printf("Please release the session and check the device wheather it listen or not.\n");
        break;
    case IOTC_ER_CH_NOT_ON:
        //-26 IOTC_ER_CH_NOT_ON
        printf("[Error code : %d]\n", IOTC_ER_CH_NOT_ON);
        printf("Channel isn't on.\n");
        printf("Please open it by IOTC_Session_Channel_ON() or IOTC_Session_Get_Free_Channel()\n");
        printf("Retry...\n");
        break;
    case IOTC_ER_SESSION_NO_FREE_CHANNEL:
        //-31 IOTC_ER_SESSION_NO_FREE_CHANNEL
        printf("[Error code : %d]\n", IOTC_ER_SESSION_NO_FREE_CHANNEL);
        printf("All channels are occupied.\n");
        printf("Please release some channel.\n");
        break;
    case IOTC_ER_TCP_TRAVEL_FAILED:
        //-32 IOTC_ER_TCP_TRAVEL_FAILED
        printf("[Error code : %d]\n", IOTC_ER_TCP_TRAVEL_FAILED);
        printf("Device can't connect to Master.\n");
        printf("Don't let device use proxy.\n");
        printf("Close firewall of device.\n");
        printf("Or open device's TCP port 80, 443, 8080, 8000, 21047.\n");
        break;
    case IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED:
        //-33 IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED
        printf("[Error code : %d]\n", IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED);
        printf("Device can't connect to server by TCP.\n");
        printf("Don't let server use proxy.\n");
        printf("Close firewall of server.\n");
        printf("Or open server's TCP port 80, 443, 8080, 8000, 21047.\n");
        printf("Retry...\n");
        break;
    case IOTC_ER_NO_PERMISSION:
        //-40 IOTC_ER_NO_PERMISSION
        printf("[Error code : %d]\n", IOTC_ER_NO_PERMISSION);
        printf("This UID's license doesn't support TCP.\n");
        break;
    case IOTC_ER_NETWORK_UNREACHABLE:
        //-41 IOTC_ER_NETWORK_UNREACHABLE
        printf("[Error code : %d]\n", IOTC_ER_NETWORK_UNREACHABLE);
        printf("Network is unreachable.\n");
        printf("Please check your network.\n");
        printf("Retry...\n");
        break;
    case IOTC_ER_FAIL_SETUP_RELAY:
        //-42 IOTC_ER_FAIL_SETUP_RELAY
        printf("[Error code : %d]\n", IOTC_ER_FAIL_SETUP_RELAY);
        printf("Client can't connect to a device via Lan, P2P, and Relay mode\n");
        break;
    case IOTC_ER_NOT_SUPPORT_RELAY:
        //-43 IOTC_ER_NOT_SUPPORT_RELAY
        printf("[Error code : %d]\n", IOTC_ER_NOT_SUPPORT_RELAY);
        printf("Server doesn't support UDP relay mode.\n");
        printf("So client can't use UDP relay to connect to a device.\n");
        break;

    default:
        printf("[Unknow error code : %d]\n", error);
        break;
    }
}

//########################################################
//# Print IOTC & AV version
//########################################################
static void PrintVersion()
{
    const char *iotc_ver = IOTC_Get_Version_String();
    const char *av_ver = avGetAVApiVersionString();
    printf("IOTCAPI version[%s] AVAPI version[%s]\n", iotc_ver, av_ver);
}


//########################################################
//# Enable / Disable live stream to AV client
//########################################################
static void RegEditClient(int sid, int av_index)
{
    AV_Client *p = &gClientInfo[sid];
    p->av_index = av_index;
}

static void RegEditClientToVideo(int sid, int av_index)
{
    AV_Client *p = &gClientInfo[sid];
    p->av_index = av_index;
    p->enable_video = 1;
}

static void UnRegEditClientFromVideo(int sid)
{
    AV_Client *p = &gClientInfo[sid];
    p->enable_video = 0;
}

static void RegEditClientToAudio(int sid, int av_index)
{
    AV_Client *p = &gClientInfo[sid];
    p->enable_audio = 1;
}

static void UnRegEditClientFromAudio(int sid)
{
    AV_Client *p = &gClientInfo[sid];
    p->enable_audio = 0;
}

static void RegEditClientStreamMode(int sid, int stream_mode)
{
    AV_Client *p = &gClientInfo[sid];
    p->two_way_stream = stream_mode;
}

static int GetSidFromAvIndex(int av_index)
{
    for (int i = 0 ; i < MAX_CLIENT_NUMBER; i++) {
        if (gClientInfo[i].av_index == av_index) {
            return i;
        }
    }
    return -1;
}


//########################################################
//# Callback functions for avServStartEx()
//########################################################
static int ExTokenDeleteCallBackFn(int av_index, const char *identity)
{
    //not implement in this sample
    return 0;
}

static int ExTokenRequestCallBackFn(int av_index, const char *identity, const char *identity_description, char *token, unsigned int token_length)
{
    //not implement in this sample
    return 0;
}

static void ExGetIdentityArrayCallBackFn(int av_index, avServSendIdentityArray send_identity_array)
{
    //not implement in this sample
}

static int ExChangePasswordCallBackFn(int av_index, const char *account, const char *old_password, const char *new_password, const char *new_iotc_authkey)
{
    //not implement in this sample
    return 0;
}

static void ExAbilityRequestFn(int av_index, avServSendAbility send_ability)
{
    //not implement in this sample
}

static int ExJsonRequestFn(int av_index, const char *func, const NebulaJsonObject *json_args, NebulaJsonObject **response)
{
    int ret = 0, value = 0, status_code = 0;
    const NebulaJsonObject *json_value = NULL;
    printf("ExJsonRequestFn %s\n", func);
    int sid = GetSidFromAvIndex(av_index);
    if (sid < 0) {
        printf("No coresponding SID for index:%d !!", av_index);
        return 400;
    }

    if(strcmp(func, "startVideo")==0) {
        ret = Nebula_Json_Obj_Get_Sub_Obj(json_args, "value", &json_value);
        if (ret != NEBULA_ER_NoERROR || json_value == NULL) {
            printf("Unable to get value object\n");
            return 400;
        }

        ret = Nebula_Json_Obj_Get_Bool(json_value, &value);
        if (ret == NEBULA_ER_NoERROR) {
            printf("func is [%s] value is [%d]\n", func, value);
            if(value == 1){
                RegEditClientToVideo(sid, av_index);
            } else {
                UnRegEditClientFromVideo(sid);
            }
            status_code = 200;
        } else {
            printf("Unable to get correct value\n");
            status_code = 400;
        }
    } else if(strcmp(func, "startAudio")==0) {
        ret = Nebula_Json_Obj_Get_Sub_Obj(json_args, "value", &json_value);
        if (ret != NEBULA_ER_NoERROR || json_value == NULL) {
            printf("Unable to get value object\n");
            return 400;
        }

        ret = Nebula_Json_Obj_Get_Bool(json_value, &value);
        if (ret == NEBULA_ER_NoERROR) {
            printf("func is [%s] value is [%d]\n", func, value);
            if(value == 1){
                RegEditClientToAudio(sid, av_index);
            } else {
                UnRegEditClientFromAudio(sid);
            }
            status_code = 200;
        } else {
            printf("Unable to get correct value\n");
            status_code = 400;
        }
    } else if(strcmp(func, "playbackControl")==0) {
        const NebulaJsonObject *json_ctrl = NULL;
        const NebulaJsonObject *json_filename = NULL;
        Nebula_Json_Obj_Get_Sub_Obj(json_args, "ctrl", &json_ctrl);
        Nebula_Json_Obj_Get_Sub_Obj(json_args, "fileName", &json_filename);

        if (json_ctrl == NULL || json_filename == NULL) {
            printf("Unable to get playbackControl object!!\n");
            return 400;
        }

        int ctrl_value;
        Nebula_Json_Obj_Get_Int(json_ctrl, &ctrl_value);
        const char *filename = NULL;
        Nebula_Json_Obj_Get_String(json_filename, &filename);
        printf("func is [%s] value[%d] file[%s]\n", func, ctrl_value, filename);

        ret = HandlePlaybackControl(sid, ctrl_value, filename);
        if(ret != NEBULA_ER_NoERROR){
            status_code = 400;
        } else {
            status_code = 200;
        }
    } else if(strcmp(func, "getCameraCapability")==0){
        const char json[] = "{\"channels\":[{\"protocols\":[\"iotc-av\"],\"channelId\":0,\"lens\":{\"type\":\"normal\"},\"video\":{\"codecs\":[\"h264\"],\"averageBitrates\":[100000],\"presets\":[{\"name\":\"720p\",\"codec\":\"h264\",\"averageBitrate\":100000,\"resolution\":\"1280×720\"}]},\"audio\":{\"presets\":[{\"name\":\"pcm_8000_16_1\",\"codec\":\"pcm\",\"sampleRate\":8000,\"bitsPerSample\":16,\"channelCount\":1}]},\"speaker\":{\"presets\":[{\"name\":\"speaker_1\",\"codec\":\"pcm\",\"sampleRate\":8000,\"bitsPerSample\":16,\"channelCount\":1}]}},{\"protocols\":[\"iotc-av\"],\"channelId\":1,\"lens\":{\"type\":\"normal\"},\"video\":{\"codecs\":[\"h264\"],\"averageBitrates\":[100000],\"presets\":[{\"name\":\"720p\",\"codec\":\"h264\",\"averageBitrate\":100000,\"resolution\":\"1280×720\"},{\"name\":\"1080p\",\"codec\":\"h264\",\"averageBitrate\":500000,\"resolution\":\"1920x1080\"}]}}]}";
        ret = Nebula_Json_Obj_Create_From_String(json, response);
        printf("create getCameraCapability response[%d]\n", ret);
        if(ret != 0){
            status_code = 400;
        } else {
            status_code = 200;
        }
    } else if(strcmp(func, "startSpeaker")==0) {
        int enable_speaker = 0;
        Nebula_Json_Obj_Get_Sub_Obj_Bool(json_args, "value", &enable_speaker);
        ret = HandleSpeakerControl(sid, enable_speaker);
        if(ret != 0){
            status_code = 400;
        } else {
            status_code = 200;
        }
    } else {
        printf("avIndex[%d], recv unknown function request[%s]\n", av_index, func);
        status_code = 400;
    }

    return status_code;
}

int StartAvServer(int sid, unsigned char chid, unsigned int timeout_sec, unsigned int resend_buf_size_kbyte, int *stream_mode)
{
    struct st_SInfoEx session_info;

    AVServStartInConfig av_start_in_config;
    AVServStartOutConfig av_start_out_config;

    memset(&av_start_in_config, 0, sizeof(AVServStartInConfig));
    av_start_in_config.cb = sizeof(AVServStartInConfig);
    av_start_in_config.iotc_session_id = sid;
    av_start_in_config.iotc_channel_id = chid;
    av_start_in_config.timeout_sec = timeout_sec;
    av_start_in_config.password_auth = NULL;
    av_start_in_config.token_auth = NULL;
    av_start_in_config.server_type = SERVTYPE_STREAM_SERVER;
    av_start_in_config.resend = ENABLE_RESEND;
    av_start_in_config.token_delete = ExTokenDeleteCallBackFn;
    av_start_in_config.token_request = ExTokenRequestCallBackFn;
    av_start_in_config.identity_array_request = ExGetIdentityArrayCallBackFn;
    av_start_in_config.change_password_request = ExChangePasswordCallBackFn;
    av_start_in_config.ability_request = ExAbilityRequestFn;
    av_start_in_config.json_request = ExJsonRequestFn;

    if (ENABLE_DTLS)
        av_start_in_config.security_mode = AV_SECURITY_DTLS; // Enable DTLS, otherwise use AV_SECURITY_SIMPLE
    else
        av_start_in_config.security_mode = AV_SECURITY_SIMPLE;

    av_start_out_config.cb = sizeof(av_start_out_config);

    int av_index = avServStartEx(&av_start_in_config, &av_start_out_config);

    if (av_index < 0) {
        printf("avServStartEx failed!! SID[%d] code[%d]!!!\n", sid, av_index);
        return -1;
    }
    session_info.size = sizeof(session_info);
    if (IOTC_Session_Check_Ex(sid, &session_info) == IOTC_ER_NoERROR) {
        char *mode[3] = {"P2P", "RLY", "LAN"};

        if (isdigit(session_info.RemoteIP[0]))
            printf("Client is from[IP:%s, Port:%d] Mode[%s] VPG[%d:%d:%d] VER[%X] NAT[%d] AES[%d]\n", \
            session_info.RemoteIP, session_info.RemotePort, mode[(int)session_info.Mode], session_info.VID, session_info.PID, session_info.GID, session_info.IOTCVersion, session_info.LocalNatType, session_info.isSecure);
    }
    printf("avServStartEx OK, SID[%d] avIndex[%d], resend[%d] two_way_streaming[%d] auth_type[%d]\n", \
        sid, av_index, av_start_out_config.resend, av_start_out_config.two_way_streaming, av_start_out_config.auth_type);
    if(stream_mode){
        *stream_mode = av_start_out_config.two_way_streaming;
    }
    avServSetResendSize(av_index, resend_buf_size_kbyte);

    return av_index; 
}


//########################################################
//#Thread - Send live streaming
//########################################################
static void *ThreadVideoFrameData(void *arg)
{
    unsigned int total_count = 0;
    float hF = 0.0, lF= 0.0, total_fps = 0;
    long take_sec = 0, take_us = 0, send_frame_us = 0;
    int fps_count = 0, round = 0, frame_rate = FPS, sleep_us = 1000000/frame_rate;
    int i = 0, av_index = 0, enable_video = 0, send_frame_out = 0, size = 0, ret = 0, lock_ret = 0;

    char buf[VIDEO_BUF_SIZE];
    struct timeval tv, tv2;
    struct timeval tv_start, tv_end;
    FILE *fp = NULL;
    FRAMEINFO_t frame_info;

    //Adding
    unsigned char iFrmaePrefix[] = {0x00, 0x00, 0x00, 0x01, 0x67};
    unsigned char pFrmaePrefix[] = {0x00, 0x00, 0x00, 0x01, 0x41};
    unsigned char *f;
    int fsize;
    struct stat s;
    int fd;
    int starter264nal = sizeof(iFrmaePrefix) / sizeof(unsigned char);
    unsigned char *head;    
    unsigned char *tmp;
    int record = -1;
    int ipFrame = -1; /* 0:I-Frame|1:P-Frame */
    
    size_t size_ret = 0;
    int frame_ret = -1;

    fd = open (gLiveIframePath, O_RDONLY);
    if(fd == -1){
        printf("%s: Video File \'%s\' open error!!\n", __func__, gLiveIframePath);
        printf("[Vidio] is DISABLED!!\n");
        printf("%s: exit\n", __func__);
        pthread_exit(0);
    }
    
    /* Get the size of the file. */
    int status = fstat (fd, & s);
    if(status == -1){ /* Get file size */
        printf("fstat");
        exit(0);
    }
    
    fsize = s.st_size;
    unsigned char buf_ret[fsize];

    int wfd = -1;    

    f = (unsigned char *) mmap (0, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    printf("%s start OK\n", __func__);
    printf("[Video] is ENABLED!!\n");

    while(gProgressRun){
        for (int i = 0; i < fsize; i++){
            head = f+i;
            if (memcmp(head, pFrmaePrefix, starter264nal) == 0){
                if (record>-1){
                        frame_ret = ipFrame;
                size_ret = 1;
                    memcpy(buf_ret, tmp, i-record);
                }
            record = i;
            tmp = head;
            ipFrame = 1;
            }

            else if (memcmp(head, iFrmaePrefix, starter264nal) == 0){
                if (record>-1){
                        frame_ret = ipFrame;
                        size_ret = 1;
                        memcpy(buf_ret, tmp, i-record);
                }
            record = i;
            tmp = head;
            ipFrame = 0;
            }

            /* Last loop, only run once and break */
            else if (i == (fsize-1)){
                frame_ret = ipFrame;
                size_ret = 1;
                memcpy(buf_ret, tmp, i-record+1);
            }

            else
                ;
            if (size_ret>0){
                /* Input for TUTK... */
                if (frame_ret == 0){
                    /* I-Frame Process */
                    // *** set Video Frame info here ***
                    memset(&frame_info, 0, sizeof(frame_info));
                    frame_info.codec_id = MEDIA_CODEC_VIDEO_H264;
                    frame_info.flags = IPC_FRAME_FLAG_IFRAME;
                }
                else{
                    /* P-Frame Process */
                    // *** set Video Frame info here ***
                    memset(&frame_info, 0, sizeof(frame_info));
                    frame_info.codec_id = MEDIA_CODEC_VIDEO_H264;
                    frame_info.flags = IPC_FRAME_FLAG_PBFRAME;
                }
                // fwrite(buf_ret, 1, fsize, stdout);
                /* TUTK Program starts here */
                frame_info.timestamp = GetTimeStampMs();
                send_frame_out = 0;
                take_sec = 0, take_us = 0, send_frame_us = 0;

                if(fps_count == 0)
                    gettimeofday(&tv, NULL);

                for(i = 0 ; i < MAX_CLIENT_NUMBER; i++){
                    //get reader lock
                    lock_ret = pthread_rwlock_rdlock(&gClientInfo[i].lock);
                    if(lock_ret)
                        printf("Acquire SID %d rdlock error, ret = %d\n", i, lock_ret);

                    av_index = gClientInfo[i].av_index;
                    enable_video = gClientInfo[i].enable_video;

                    //release reader lock
                    lock_ret = pthread_rwlock_unlock(&gClientInfo[i].lock);
                    if(lock_ret)
                        printf("Acquire SID %d rdlock error, ret = %d\n", i, lock_ret);

                    if(av_index < 0 || enable_video == 0){
                        continue;
                    }

                    // Send Video Frame to av-idx and know how many time it takes
                    frame_info.onlineNum = gOnlineNum;
                    gettimeofday(&tv_start, NULL);
                    ret = avSendFrameData(av_index, buf_ret, fsize, &frame_info, sizeof(frame_info));
                    gettimeofday(&tv_end, NULL);

                    take_sec = tv_end.tv_sec-tv_start.tv_sec, take_us = tv_end.tv_usec-tv_start.tv_usec;
                    if(take_us < 0){
                        take_sec--;
                        take_us += 1000000;
                    }
                    send_frame_us += take_us;
                    total_count++;

                    if(ret == AV_ER_EXCEED_MAX_SIZE){ // means data not write to queue, send too slow, I want to skip it
                        usleep(5000);
                        continue;
                    }
                    else if(ret == AV_ER_SESSION_CLOSE_BY_REMOTE){
                        printf("%s AV_ER_SESSION_CLOSE_BY_REMOTE SID[%d] avIndex[%d]\n", __func__, i, av_index);
                        UnRegEditClientFromVideo(i);
                        continue;
                    }
                    else if(ret == AV_ER_REMOTE_TIMEOUT_DISCONNECT){
                        printf("%s AV_ER_REMOTE_TIMEOUT_DISCONNECT SID[%d] avIndex[%d]\n", __func__, i, av_index);
                        UnRegEditClientFromVideo(i);
                        continue;
                    }
                    else if(ret == IOTC_ER_INVALID_SID){
                        printf("%s Session cant be used anymore SID[%d] avIndex[%d]\n", __func__, i, av_index);
                        UnRegEditClientFromVideo(i);
                        continue;
                    }
                    else if(ret < 0){
                        printf("%s SID[%d] avIndex[%d] error[%d]\n", __func__, i, av_index, ret);
                        UnRegEditClientFromVideo(i);
                    }

                    send_frame_out = 1;
                }

                if(1 == send_frame_out && fps_count++ >= frame_rate){
                    round++;
                    gettimeofday(&tv2, NULL);
                    long sec = tv2.tv_sec-tv.tv_sec, usec = tv2.tv_usec-tv.tv_usec;
                    if(usec < 0){
                        sec--;
                        usec += 1000000;
                    }
                    usec += (sec*1000000);

                    long one_frame_use_time = usec / fps_count;
                    float fps = (float)1000000/one_frame_use_time;
                    if(fps > hF) hF = fps;
                    if(lF == 0.0) lF = fps;
                    else if(fps < lF) lF = fps;
                    printf("Fps = %f R[%d]\n", fps, round);
                    fps_count = 0;
                    total_fps += fps;
                }

                // notice the frames sending time for more specific frame rate control
                if( sleep_us > send_frame_us )
                    usleep(sleep_us-send_frame_us);

                /* Flush Param */
                memset(buf_ret, 0, fsize);
                size_ret = 0;
            }
        }


    /*
    fp = fopen(gLiveIframePath, "rb");
    
    if(fp == NULL){
        printf("%s: Video File \'%s\' open error!!\n", __func__, gLiveIframePath);
        printf("[Vidio] is DISABLED!!\n");
        printf("%s: exit\n", __func__);
        pthread_exit(0);
    }

    // input file only one I frame for test
    size = fread(buf, 1, VIDEO_BUF_SIZE, fp);
    fclose(fp);
    if(size <= 0){
        printf("%s: Video File \'%s\' read error!!\n", __func__, gLiveIframePath);
        printf("[Vidio] is DISABLED!!\n");
        printf("%s: exit\n", __func__);
        pthread_exit(0);
    }
    */

    }
    printf("[%s] exit High/Low [%f/%f] AVG[%f] totalCnt[%d]\n", __func__, hF, lF, (float)total_fps/round, total_count);
    pthread_exit(0);
}

static void *ThreadAudioFrameData(void *arg)
{
    FILE *fp=NULL;
    char buf[AUDIO_BUF_SIZE];
    int frame_rate = AUDIO_FPS;
    int sleep_ms = 1000000/frame_rate;
    FRAMEINFO_t frame_info;

    if(gAudioFilePath[0]=='\0')
    {
        printf("[Audio] is DISABLED!!\n");
        printf("%s: exit\n", __func__);
        pthread_exit(0);
    }
    fp = fopen(gAudioFilePath, "rb");
    if(fp == NULL)
    {
        printf("%s: Audio File \'%s\' open error!!\n", __func__, gAudioFilePath);
        printf("[Audio] is DISABLED!!\n");
        printf("%s: exit\n", __func__);
        pthread_exit(0);
    }

    // *** set audio frame info here ***
    memset(&frame_info, 0, sizeof(frame_info));
    frame_info.codec_id = AUDIO_CODEC;
    frame_info.flags = (AUDIO_SAMPLE_8K << 2) | (AUDIO_DATABITS_16 << 1) | AUDIO_CHANNEL_MONO;

    printf("%s start OK\n", __func__);
    printf("[Audio] is ENABLED!!\n");

    while(gProgressRun)
    {
        int i;
        int ret;
        int size = fread(buf, 1, AUDIO_FRAME_SIZE, fp);
        if(size <= 0)
        {
            printf("rewind audio\n");
            rewind(fp);
            continue;
        }

        frame_info.timestamp = GetTimeStampMs();

        for(i = 0 ; i < MAX_CLIENT_NUMBER; i++)
        {
            //get reader lock
            int lock_ret = pthread_rwlock_rdlock(&gClientInfo[i].lock);
            if(lock_ret)
                printf("Acquire SID %d rdlock error, ret = %d\n", i, lock_ret);
            if(gClientInfo[i].av_index < 0 || gClientInfo[i].enable_audio == 0)
            {
                //release reader lock
                lock_ret = pthread_rwlock_unlock(&gClientInfo[i].lock);
                if(lock_ret)
                    printf("Acquire SID %d rdlock error, ret = %d\n", i, lock_ret);
                continue;
            }
            int av_index = gClientInfo[i].av_index;

            // send audio data to av-idx
            ret = avSendAudioData(av_index, buf, size, &frame_info, sizeof(FRAMEINFO_t));
            //release reader lock
            lock_ret = pthread_rwlock_unlock(&gClientInfo[i].lock);
            if(lock_ret)
                printf("Acquire SID %d rdlock error, ret = %d\n", i, lock_ret);

            if(ret == AV_ER_SESSION_CLOSE_BY_REMOTE)
            {
                printf("%s: AV_ER_SESSION_CLOSE_BY_REMOTE SID[%d] avIndex[%d]\n", __func__, i, av_index);
                UnRegEditClientFromAudio(i);
            }
            else if(ret == AV_ER_REMOTE_TIMEOUT_DISCONNECT)
            {
                printf("%s: AV_ER_REMOTE_TIMEOUT_DISCONNECT SID[%d] avIndex[%d]\n", __func__, i, av_index);
                UnRegEditClientFromAudio(i);
            }
            else if(ret == IOTC_ER_INVALID_SID)
            {
                printf("%s Session cant be used anymore SID[%d] avIndex[%d]\n", __func__, i, av_index);
                UnRegEditClientFromAudio(i);
            }
            else if(ret == AV_ER_EXCEED_MAX_SIZE)
            {
                printf("%s AV_ER_EXCEED_MAX_SIZE SID[%d] avIndex[%d]\n", __func__, i, av_index);
            }
            else if(ret < 0)
            {
                printf("%s SID[%d] avIndex[%d] error[%d]\n", __func__, i, av_index, ret);
                UnRegEditClientFromAudio(i);
            }
        }

        usleep(sleep_ms);
    }

    fclose(fp);

    printf("[%s] exit\n",  __func__);

    pthread_exit(0);
}


//########################################################
//# Start AV server and recv IOCtrl cmd for every new av idx
//########################################################
static void *ThreadForAVServerStart(void *arg)
{
    int sid = *(int *)arg;
    free(arg);

    int chid = AV_LIVE_STREAM_CHANNEL;
    int timeout_sec = 30;
    int stream_mode = 0;
    int resend_buf_size_kbytes = 1024;

    printf("%s SID[%d]\n", __func__, sid);

    int av_index = StartAvServer(sid, chid, timeout_sec, resend_buf_size_kbytes, &stream_mode);
    if (av_index < 0) {
        printf("StartAvServer SID[%d] ret[%d]!!!\n", sid, av_index);
        goto EXIT;
    }

    gOnlineNum++;
    RegEditClient(sid, av_index);
    RegEditClientStreamMode(sid, stream_mode);
    
    struct st_SInfoEx session_info;
    session_info.size = sizeof(struct st_SInfoEx);
    while(1) {
        int ret = IOTC_Session_Check_Ex(sid, &session_info);
        if(ret != IOTC_ER_NoERROR){
            break;
        }
        sleep(1);
    }

EXIT:
    UnRegEditClientFromVideo(sid);
    UnRegEditClientFromAudio(sid);
    RegEditClientPlaybackMode(sid, PLAYBACK_STOP);

    if(av_index >= 0){
        printf("avServStop[%d]\n", av_index);
        avServStop(av_index);
        gOnlineNum--;
    }
    printf("IOTC_Session_Close[%d]\n", sid);
    IOTC_Session_Close(sid);
    printf("SID[%d], avIndex[%d], %s exit!!\n", sid, av_index, __func__);
    pthread_exit(0);
}

#ifdef ENABLE_EMULATOR
static int  gCacheSize = 0;
static char gIFrameCache[VIDEO_BUF_SIZE] = {0};

int StreamoutSetCacheIFrame(char* buf, int size)
{
    if(size > VIDEO_BUF_SIZE){
        printf("Streamout_SetCacheIFrame gIFrameCache too small\n");
        return -1;
    }

    gCacheSize = size;
    memcpy(gIFrameCache, buf, size);

    return 0;
}

int StreamoutGetCacheIFrame(char* buf)
{
    if(gCacheSize == 0)
        return -1;

    buf = gIFrameCache;

    return gCacheSize;
}

int StreamoutSendVideoFunc(int stream_id, int timestamp, char* buf, int size, int frame_type)
{
    //printf("Video streamID[%d] timestamp[%d] frameType[%s] size[%d]\n", streamID, timestamp, frameType == IPC_FRAME_FLAG_IFRAME ? "I" : "P", size);
    int i = 0, av_index = 0, enable_video = 0, ret = 0, lock_ret = 0;
    FRAMEINFO_t frame_info;
    char *video_buf = buf;
    int video_size = size;

    if(frame_type == IPC_FRAME_FLAG_IFRAME){
        StreamoutSetCacheIFrame(buf, size);
    }

    // *** set Video Frame info here ***
    memset(&frame_info, 0, sizeof(FRAMEINFO_t));
    frame_info.codec_id = MEDIA_CODEC_VIDEO_H264;
    frame_info.flags = frame_type;
    frame_info.timestamp = timestamp;

    for(i = 0 ; i < MAX_CLIENT_NUMBER; i++) {
        //get reader lock
        lock_ret = pthread_rwlock_rdlock(&gClientInfo[i].lock);
        if(lock_ret)
            printf("Acquire SID %d rdlock error, ret = %d\n", i, lock_ret);

        av_index = gClientInfo[i].av_index;
        enable_video = gClientInfo[i].enable_video;

        //release reader lock
        lock_ret = pthread_rwlock_unlock(&gClientInfo[i].lock);
        if(lock_ret)
            printf("Acquire SID %d rdlock error, ret = %d\n", i, lock_ret);

        if(av_index < 0 || enable_video == 0) {
            continue;
        }
        if(gClientInfo[i].wait_key_frame == 1 && frame_type == IPC_FRAME_FLAG_PBFRAME) {
            continue;
        }

        frame_info.onlineNum = gOnlineNum;
        ret = avSendFrameData(av_index, video_buf, video_size, &frame_info, sizeof(frame_info));

        if(ret == AV_ER_EXCEED_MAX_SIZE) {
            lock_ret = pthread_rwlock_wrlock(&gClientInfo[i].lock);
            if(lock_ret)
                printf("Acquire SID %d rwlock error, ret = %d\n", i, lock_ret);

            gClientInfo[i].wait_key_frame = 1;

            lock_ret = pthread_rwlock_unlock(&gClientInfo[i].lock);
            if(lock_ret)
                printf("Release SID %d rwlock error, ret = %d\n", i, lock_ret);
            continue;
        } else if(ret == AV_ER_SESSION_CLOSE_BY_REMOTE) {
            printf("%s AV_ER_SESSION_CLOSE_BY_REMOTE SID[%d] avIndex[%d]\n", __func__, i, av_index);
            UnRegEditClientFromVideo(i);
            continue;
        } else if(ret == AV_ER_REMOTE_TIMEOUT_DISCONNECT) {
            printf("%s AV_ER_REMOTE_TIMEOUT_DISCONNECT SID[%d] avIndex[%d]\n", __func__, i, av_index);
            UnRegEditClientFromVideo(i);
            continue;
        } else if(ret == IOTC_ER_INVALID_SID) {
            printf("%s Session cant be used anymore SID[%d] avIndex[%d]\n", __func__, i, av_index);
            UnRegEditClientFromVideo(i);
            continue;
        } else if(ret == AV_ER_DASA_CLEAN_BUFFER) {
            printf("avIndex[%d] need to do clean buffer\n", av_index);
            lock_ret = pthread_rwlock_wrlock(&gClientInfo[i].lock);
            if(lock_ret)
                printf("Acquire SID %d rwlock error, ret = %d\n", i, lock_ret);

            gClientInfo[i].do_clean_buffer = 1;
            gClientInfo[i].do_clean_buffer_done = 0;
            gClientInfo[i].wait_key_frame = 1;

            lock_ret = pthread_rwlock_unlock(&gClientInfo[i].lock);
            if(lock_ret)
                printf("Release SID %d rwlock error, ret = %d\n", i, lock_ret);
        } else if(ret < 0) {
            printf("%s avSendFrameData err[%d] SID[%d] avIndex[%d]\n", __func__, ret, i, av_index);
            UnRegEditClientFromVideo(i);
        } else {
            gClientInfo[i].wait_key_frame = 0;
        }
    }

    return 0;
}

int StreamoutSendAudioFunc(int timestamp, char* buf, int size)
{
    //printf("Audio timestamp[%d] size[%d]\n", timestamp, size);
    int i = 0, ret = 0, av_index = 0;
    FRAMEINFO_t frame_info;

    // *** set audio frame info here ***
    memset(&frame_info, 0, sizeof(frame_info));
    frame_info.codec_id = AUDIO_CODEC;
    frame_info.flags = (AUDIO_SAMPLE_8K << 2) | (AUDIO_DATABITS_16 << 1) | AUDIO_CHANNEL_MONO;
    frame_info.timestamp = timestamp;
    frame_info.onlineNum = gOnlineNum;

    for(i = 0 ; i < MAX_CLIENT_NUMBER; i++) {
        //get reader lock
        int lock_ret = pthread_rwlock_rdlock(&gClientInfo[i].lock);
        if(lock_ret)
            printf("Acquire SID %d rdlock error, ret = %d\n", i, lock_ret);
        if(gClientInfo[i].av_index < 0 || gClientInfo[i].enable_audio == 0) {
            //release reader lock
            lock_ret = pthread_rwlock_unlock(&gClientInfo[i].lock);
            if(lock_ret)
                printf("Acquire SID %d rdlock error, ret = %d\n", i, lock_ret);
            continue;
        }
        av_index = gClientInfo[i].av_index;

        // send audio data to av-idx
        ret = avSendAudioData(av_index, buf, size, &frame_info, sizeof(frame_info));
        //release reader lock
        lock_ret = pthread_rwlock_unlock(&gClientInfo[i].lock);
        if(lock_ret)
            printf("Acquire SID %d rdlock error, ret = %d\n", i, lock_ret);

        //printf("avIndex[%d] size[%d]\n", gClientInfo[i].avIndex, size);
        if(ret == AV_ER_SESSION_CLOSE_BY_REMOTE) {
            printf("%s AV_ER_SESSION_CLOSE_BY_REMOTE SID[%d] avIndex[%d]\n", __func__, i, av_index);
            UnRegEditClientFromAudio(i);
        } else if(ret == AV_ER_REMOTE_TIMEOUT_DISCONNECT) {
            printf("%s AV_ER_REMOTE_TIMEOUT_DISCONNECT SID[%d] avIndex[%d]\n", __func__, i, av_index);
            UnRegEditClientFromAudio(i);
        } else if(ret == IOTC_ER_INVALID_SID) {
            printf("%s Session cant be used anymore SID[%d] avIndex[%d]\n", __func__, i, av_index);
            UnRegEditClientFromAudio(i);
        } else if(ret < 0) {
            printf("%s avSendAudioData error[%d] SID[%d] avIndex[%d]\n", __func__, ret, i, av_index);
            UnRegEditClientFromAudio(i);
        }
    }

    return 0;
}
#endif

//########################################################
//#  Initialize / Deinitialize client list of AV server
//########################################################
static void InitAVInfo()
{
    int i;
    for (i = 0; i < MAX_CLIENT_NUMBER; i++) {
        memset(&gClientInfo[i], 0, sizeof(AV_Client));
        gClientInfo[i].av_index = -1;
        gClientInfo[i].enable_record_video = PLAYBACK_STOP;
        pthread_rwlock_init(&(gClientInfo[i].lock), NULL);
    }
}

static void DeInitAVInfo()
{
    int i;
    for (i = 0; i < MAX_CLIENT_NUMBER; i++) {
        memset(&gClientInfo[i], 0, sizeof(AV_Client));
        gClientInfo[i].av_index = -1;
        pthread_rwlock_destroy(&gClientInfo[i].lock);
    }
}

//########################################################
//# identity_handler callback function
//########################################################
static void IdentityHandle(NebulaDeviceCtx *device, const char *identity, char *psk, unsigned int psk_size)
{
    int ret = GetPskFromFile(identity, gUserIdentitiesFilePath, psk, psk_size);
    if (ret != 200) {
        printf("[%s] get psk fail\n", __func__);
    }
}

//########################################################
//# command_handler callback function
//########################################################
static int CommandHandle(NebulaDeviceCtx *device, const char *identity, const char *func, const NebulaJsonObject *json_args, NebulaJsonObject **json_response)
{
    printf("[%s] %s\n", __func__, func);

    int ret = 0;
    char resp[1024] = {0};
    NebulaJsonObject *response = NULL;

    printf("arg %s\n", Nebula_Json_Obj_To_String(json_args));
    if (strcmp("getNightMode", func) == 0) {
        // The profile or document might describe getNightMode as
        // { 
        //   "func":"getNightMode",
        //   "return": {
        //     "value":"Int"
        //   }
        // }
        // When the value of night mode is 10, please make a JSON response as { "value": 10 }
        // There is no need to add key "content" here   
        int night_mode = 0;
        sprintf(resp, "{\"value\":%d}", night_mode);
        printf("%s: send response[%s]\n", __func__, resp);

        ret = Nebula_Json_Obj_Create_From_String(resp, &response);

        if (response == NULL) {
            printf("%s: Invalid response error ret [%d]!!\n", __func__, ret);
            return 400;
        }

        *json_response = response;
        return 200;
    }else if(strcmp(func, "queryEventList")==0){
        const NebulaJsonObject *json_ctrl;
        printf("%s \n", Nebula_Json_Obj_To_String(json_args));
        Nebula_Json_Obj_Get_Sub_Obj(json_args, "startTime", &json_ctrl);
        int starttime = 0;
        ret = Nebula_Json_Obj_Get_Int(json_ctrl, &starttime);
        
        Nebula_Json_Obj_Get_Sub_Obj(json_args, "endTime", &json_ctrl);
        int endtime = 0;
        Nebula_Json_Obj_Get_Int(json_ctrl, &endtime);

        Nebula_Json_Obj_Get_Sub_Obj(json_args, "eventType", &json_ctrl);
        const char *eventType = NULL;
        Nebula_Json_Obj_Get_String(json_ctrl, &eventType);

        printf("startTime[%d], endtime[%d], eventType[%s]\n", starttime, endtime, eventType);

        //List the file name that match the search criteria.
        NebulaJsonObject *response = NULL;
        GetRecordFileList(starttime, endtime, eventType, &response);
        printf("Resp[%s]\n", Nebula_Json_Obj_To_String(response));
        if (response == NULL) {
            printf("%s: Empty response error!!\n", __func__);
            return 400;
        }

        *json_response = response;
        return 200;
    } else if (strcmp(func, "createCredential") == 0) {
        const char *user_identity = NULL;
        const char *mode = NULL;
        char *credential = NULL;

        if (strcmp(identity, "admin") != 0) {
            printf("[%s] %s is not from admin ,return 403(Forbidden)\n", __func__, func);
            return 403;
        }

        ret = Nebula_Json_Obj_Get_Sub_Obj_String(json_args, "identity", &user_identity);
        if (ret != NEBULA_ER_NoERROR) {
            printf("[%s] identity is not in %s ,return 400(Bad Request)\n", __func__, func);
            return 400;
        }

        ret = Nebula_Json_Obj_Get_Sub_Obj_String(json_args, "createMode", &mode);
        if (ret != NEBULA_ER_NoERROR) {
            printf("[%s] createMode is not in %s ,return 400(Bad Request)\n", __func__, func);
            return 400;
        }
        ret = GetCredential(gUdid, gSecretId, user_identity, mode, gUserIdentitiesFilePath, &credential);
        if (ret == 200) {
            sprintf(resp, "{\"credential\":\"%s\"}", credential);
            printf("Resp[%s]\n", resp);
            free(credential);
            Nebula_Json_Obj_Create_From_String(resp, &response);
            *json_response = response;
        }
        return ret;
    } else if (strcmp(func, "deleteCredential") == 0) {
        const char *user_identity = NULL;

        if (strcmp(identity, "admin") != 0) {
            printf("[%s] %s is not from admin ,return 403(Forbidden)\n", __func__, func);
            return 403;
        }

        ret = Nebula_Json_Obj_Get_Sub_Obj_String(json_args, "identity", &user_identity);
        if (ret != NEBULA_ER_NoERROR) {
            printf("[%s] identity is not in %s ,return 400(Bad Request)\n", __func__, func);
            return 400;
        }

        return DeleteCredential(user_identity, gUserIdentitiesFilePath);
    } else if (strcmp(func, "getAllIdentities") == 0) {
        char *identities_json_str = NULL;

        if (strcmp(identity, "admin") != 0) {
            printf("[%s] %s is not from admin ,return 403(Forbidden)\n", __func__, func);
            return 403;
        }

        ret = GetAllIdentitiesFromFile(gUserIdentitiesFilePath, &identities_json_str);
        if (ret == 200) {
            printf("Resp[%s]\n", identities_json_str);
            Nebula_Json_Obj_Create_From_String(identities_json_str, &response);
            free(identities_json_str);
            *json_response = response;
        }
        return ret;
    }
    return 400;
}

//########################################################
//# settings_change_handler callback function
//########################################################
static int SettingsChangeHandle(NebulaDeviceCtx* device, const char* settings)
{
    FILE *fp = NULL;
    fp = fopen(gDefaultSettingsFilePath, "w+");
    if (fp) {
        fwrite(settings, 1, strlen(settings), fp);
        fclose(fp);
        printf("[%s] get new settings, save to %s success\n", __func__, gDefaultSettingsFilePath);
    } else {
        printf("[%s] get new settings, save to %s fail\n", __func__, gDefaultSettingsFilePath);
    }
    gSettingsExist = true;
    return 0;
}

static int DeviceLoginStateHandle(NebulaDeviceCtx* device, NebulaDeviceLoginState state)
{
    printf("in %s\n", __func__);
    printf("login state[%d]\n", state);
    return 0;
}


static int CreateStreamoutThread()
{
    int ret = 0;
#if ENABLE_EMULATOR
    ret = Emulator_Initialize(EMULATOR_MODE_SINGLESTREAM, AV_DASA_LEVEL_QUALITY_LOW, StreamoutSendVideoFunc, StreamoutSendAudioFunc);
    if(ret < 0){
        printf("Emulator_Initialize ret=%d\n", ret);
        return -1;
    }
#else
    pthread_t thread_video_frame_data_id;
    pthread_t thread_audio_frame_data_id;

    if((ret = pthread_create(&thread_video_frame_data_id, NULL, &ThreadVideoFrameData, NULL)))
    {
        printf("pthread_create ret=%d\n", ret);
        return -1;
    }
    pthread_detach(thread_video_frame_data_id);

    if((ret = pthread_create(&thread_audio_frame_data_id, NULL, &ThreadAudioFrameData, NULL)))
    {
        printf("pthread_create ret=%d\n", ret);
        return -1;
    }
    pthread_detach(thread_audio_frame_data_id);
#endif
    return 0;
}

static void DeviceServerBind(NebulaDeviceCtx *device_ctx, const char *pin_code, const char *psk, int timeout_ms)
{
    while(1) {
        int ret = Nebula_Device_Bind(device_ctx, pin_code, psk, timeout_ms, &gBindAbort);
        printf("Nebula_Device_Bind ret[%d]\n", ret);
        if (ret == NEBULA_ER_TIMEOUT)
            continue;
        else {
            break;
        }
    }
}

static void *ThreadNebulaLogin(void *arg)
{
    NebulaDeviceCtx *device_ctx = (NebulaDeviceCtx *)arg;
    int ret = 0;
    char admin_psk[MAX_NEBULA_PSK_LENGTH + 1] = {0};
    while(gProgressRun) {
        int ret = Nebula_Device_Login(device_ctx, DeviceLoginStateHandle);
        printf("Nebula_Device_Login ret[%d]\n", ret);
        if (ret == NEBULA_ER_NoERROR) {
            printf("Nebula_Device_Login success...!!\n");
            break;
        }
        sleep(1);
    }

    ret = GetPskFromFile("admin", gUserIdentitiesFilePath, admin_psk, MAX_NEBULA_PSK_LENGTH);
    if (ret != 200) {
        AppendPskToFile("admin", gUserIdentitiesFilePath, admin_psk, MAX_NEBULA_PSK_LENGTH);
    }

    switch (gBindType) {
        case SERVER_BIND:
            printf("Bind type : Server Bind\n");
            DeviceServerBind(device_ctx, gPinCode, admin_psk, 60000);
            break;
        case LOCAL_BIND:
            printf("Bind type : Local Bind\n");
            Device_LAN_WIFI_Config(gUdid, admin_psk, gSecretId, gProfileJsonObj, &gBindAbort);
            break;
        case DISABLE_BIND:
            printf("Bind type : Disable Bind\n");
            break;
        default:
            break;
    }
    return NULL;
}

static void *ThreadIotcDeviceLogin(void *arg)
{
    NebulaDeviceCtx *device_ctx = (NebulaDeviceCtx *)arg;
    while(gProgressRun){
        printf("IOTC_Device_Login_By_Nebula() start\n");
        int ret = IOTC_Device_Login_By_Nebula(device_ctx);
        printf("IOTC_Device_Login_By_Nebula() ret = %d\n", ret);
        if (ret == IOTC_ER_NoERROR) {
            printf("IOTC_Device_Login_By_Nebula success...!!\n");
            break;
        }
        sleep(1);
    }
    return NULL;
}

static int ShouldDeviceGoToSleep(){
    if(!gEnableWakeUp)
        return 0;

    struct timeval now;
    gettimeofday(&now, NULL);

    if(now.tv_sec - gNoSessionTime.tv_sec > GO_TO_SLEEP_AFTER_NO_SESSION_WITH_SEC)
        return 1;
    
    return 0;
}

static void ResetNoSessionTime(){
    gettimeofday(&gNoSessionTime, NULL);
}

static void UpdateNoSessionTime(){
    if (gOnlineNum > 0){
        ResetNoSessionTime();
    }
}

static void PrepareWakeupDataBeforeSleep(NebulaDeviceCtx *device_ctx, NebulaSocketProtocol nebula_protocol, char *wakeup_pattern, NebulaWakeUpData **data, unsigned int *data_count){
    int ret = 0;
    NebulaSleepConfig config;
    struct timeval now = {0};
    gettimeofday(&now, NULL);

    sprintf(wakeup_pattern, "WakeMeUp%u", (unsigned int)now.tv_usec);

    config.cb = sizeof(NebulaSleepConfig);
    config.wake_up_pattern = (unsigned char *)wakeup_pattern;
    config.pattern_size = strlen(wakeup_pattern);
    config.protocol = nebula_protocol;
    config.alive_interval_sec = 0; // Set 0 for default values. UDP: 25 secs, TCP: 90 secs

    do{
        ret = Nebula_Device_Get_Sleep_PacketEx(device_ctx, &config, data, data_count, 10000);
    }while(ret != NEBULA_ER_NoERROR);

}

static void WaitForWakeupPatternWhenSleeping(NebulaSocketProtocol nebula_protocol, char *wakeup_pattern, NebulaWakeUpData *data, unsigned int data_count){
    printf("Device going to sleep\n");
    int max_sock_fd = 0, i = 0;
    struct sockaddr_in *server;
    int *sock_fd;
    int recv_wakeup = 0;
    
    sock_fd = (int*)malloc(data_count*sizeof(int));
    server = (struct sockaddr_in *)malloc(data_count*sizeof(struct sockaddr_in));

    //Device sleep simulate
    for(i = 0; i<data_count; i++){
        if (nebula_protocol == NEBULA_PROTO_TCP)
            sock_fd[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        else if (nebula_protocol == NEBULA_PROTO_UDP)
            sock_fd[i] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        server[i].sin_family = AF_INET;
        server[i].sin_port = htons(data[i].port);
        inet_pton(AF_INET, data[i].ip, &server[i].sin_addr.s_addr);

        if (connect(sock_fd[i], (struct sockaddr *)&server[i], sizeof(server[i])) < 0) {
            printf("Connect to server[%d] error------fail\n", i);
            return;
        }
        if (max_sock_fd < sock_fd[i])
            max_sock_fd = sock_fd[i];
    }
    printf("Bridge server connected------ok\n");

    while(1) {
        fd_set read_fd;
        FD_ZERO(&read_fd);
        
        struct timeval timeout;
        timeout.tv_sec = data[0].login_interval_sec;
        timeout.tv_usec = 0;

        for(i = 0; i<data_count; ++i){
            send(sock_fd[i], data[i].sleep_alive_packet, data[i].packet_size, 0);
            printf("Sleep packet: send to %s:%hu size:%u interval:%u\n", data[i].ip, data[i].port, data[i].packet_size, data[i].login_interval_sec);
            FD_SET(sock_fd[i], &read_fd);
        }

        int selected_fd = select(max_sock_fd + 1, &read_fd, NULL, NULL, &timeout);
        if (selected_fd != 0) {
            for(i = 0; i<data_count; ++i){
                if(FD_ISSET(sock_fd[i], &read_fd) != 0){
                    printf("receiving...\n");
                    char buf[256] = {0};
                    int recv_size = recv(sock_fd[i], &buf, sizeof(buf), 0);

                    printf("Recv: %s size [%d]\n", buf, recv_size);

                    if (memcmp(buf, wakeup_pattern, strlen(wakeup_pattern)) == 0 ) {
                        printf("Receive Wakeup Pattern------ok\n");
                        printf("Device Wakeup\n");
                        recv_wakeup = 1;
                        break;
                    }
                }
            }
            if(recv_wakeup == 1)
                break;
        }
    }//End while

    // Release the resource
    if(sock_fd){
        for(i = 0; i<data_count; ++i){
            close(sock_fd[i]);
        }
        free(sock_fd);
    }
    if(server){
        free(server);
    }

    Nebula_Device_Free_Sleep_Packet(data);

}

static void PrintUsage()
{
    printf("#########################################################################\n");
    printf("./Nebula_Device_AV [options]\n");
    printf("[options]\n");
    printf("[M]\t-u [UDID]\t\t\tConfig UDID.\n");
    printf("[M]\t-f [profile]\t\t\tConfig profile path.\n");
    printf("[O]\t-b [l(ocal)|s(erver)|n(one)]\tBinding mode. Bind in local, bind through TUTK bind server or do not bind. Default is server.\n");
    printf("[O]\t-v \t\t\t\tEnable VSaaS server pull stream from device. Disabled by default.\n");
    printf("[O]\t-w \t\t\t\tDemo wakeup. Device will go to sleep if the process is idled 10 secs. Disabled by default.\n");
    printf("[O]\t-p \t\t\t\tEnable push notification to APP. Disabled by default.\n");
    printf("[O]\t-h \t\t\t\tShow usage\n");
}

static int ParseInputOptions(int argc, char *argv[])
{
    int option = 0;
    while((option = getopt(argc, argv, "u:f:b:hvw")) > 0){
        switch(option) {
          case 'u':
            strncpy(gUdid, optarg, sizeof(gUdid));
            gUdid[MAX_UDID_LENGTH] = '\0';
            printf("UDID %s\n", gUdid);
            break;
          case 'f':
            strncpy(gProfilePath, optarg, sizeof(gProfilePath));
            printf("Profile %s\n", gProfilePath);
            break;
          case 'b':
            if((strlen(optarg)==strlen("l") && strncmp(optarg, "l", 1) == 0)
               || (strlen(optarg)==strlen("local") && strncmp(optarg, "local", strlen("local")) == 0)) {
                gBindType = LOCAL_BIND;
            } else if((strlen(optarg)==strlen("s") && strncmp(optarg, "s", 1) == 0)
               || (strlen(optarg)==strlen("server") && strncmp(optarg, "server", strlen("server")) == 0)) {
                gBindType = SERVER_BIND;
            } else if((strlen(optarg)==strlen("n") && strncmp(optarg, "n", 1) == 0)
               || (strlen(optarg)==strlen("none") && strncmp(optarg, "none", strlen("none")) == 0)) {
                gBindType = DISABLE_BIND;
            } else {
                printf("Unknown optarg %s\n", optarg);
                return -1;
            }
            break;
          case 'v':
            gEnableVsaas = true;
            break;
          case 'p':
            gEnablePushNotification = true;
            break;
          case 'w':
            gEnableWakeUp = true;
            break;
          case 'h':
            return -1;
          default:
            printf("Unknown option %c\n", option);
            return -1;
        }
    }
    if(strlen(gUdid) == 0 || strlen(gProfilePath) == 0){
        printf("Must specify UDID and profile\n");
        return -1;
    }
    return 0;
}


//########################################################
//#     Fake VSaaS event for demo
//########################################################
#define MAX_VSAAS_DEMO_ROUND 1
#define FAKE_EVENT_TRIGGER_TIME_SEC 60
static void *ThreadFakeVsaasEvent(void *arg)
{
    // In this sample we use hard coeded file name, 
    // and trigger fake event every 60 secs periodically.
    printf("%s start\n", __func__);
    char att_json_str[256] = {0};
    unsigned long last_event_time_sec = 0;
    unsigned long current_time_sec = 0;
    int ret = 0;
    NebulaJsonObject *attr_obj = NULL;
    int demo_round = 0;

    while(gProgressRun && demo_round <= MAX_VSAAS_DEMO_ROUND) {
        if(gVsaasConfigExist) {
            current_time_sec = GetTimeStampSec();
            if (current_time_sec-last_event_time_sec > FAKE_EVENT_TRIGGER_TIME_SEC) {

                // notify VSaaS server to pull the record video. 
                sprintf(att_json_str, "{\"starttime\":\"%lu\",\"protocol\":\"tutkv4\",\"event_id\":\"%d\",\"event_file\":\"%s\",\"media_type\":\"0\"}", current_time_sec, VSAAS_EVENT_GENERAL, gRecordFile);

                printf("avServNotifyCloudRecordStream\n");
                Nebula_Json_Obj_Create_From_String(att_json_str, &attr_obj);
                ret = avServNotifyCloudRecordStream(attr_obj, 3000, NULL);
                printf("avServNotifyCloudRecordStream ret[%d]\n", ret);
                last_event_time_sec = current_time_sec;
                if (ret != AV_ER_NoERROR) {
                    break;
                }
                demo_round++;
            }
        }
        sleep(1);
    }
    printf("%s exit\n", __func__);
    pthread_exit(0);
}

//########################################################
//#     Fake Push Notification for demo
//########################################################
static void *ThreadFakePushNotification(void *arg) {
    printf("%s start\n", __func__);
    NebulaDeviceCtx *device_ctx = (NebulaDeviceCtx *)arg;
    int ret = 0;
    NebulaJsonObject *notification_obj = NULL;
    unsigned int push_abort = 0;

    ret = Nebula_Json_Obj_Create_From_String("{\"event\":\"1\"}", &notification_obj);
    if (ret != NEBULA_ER_NoERROR) {
        printf("Nebula_Json_Obj_Create_From_String[%d]\n", ret);
        return NULL;
    }

    sleep(3);

    while (gProgressRun) {
        if (gSettingsExist) {
            ret = Nebula_Device_Push_Notification(device_ctx, notification_obj, 10000, &push_abort);
            printf("Nebula_Device_Push_Notification[%d]\n", ret);
            break;
        }
        sleep(1);
    }
    Nebula_Json_Obj_Release(notification_obj);
    printf("%s exit\n", __func__);
    pthread_exit(0);
}

//########################################################
//# Callback function for avEnableVSaaS()
//########################################################
static void VsaasConfigChangedHandle(const char *vsaas_config)
{
    printf("Enter %s\n", __func__);
    printf("Get VSaaS info:\n%s\n", vsaas_config);
    //save VSaaSconfig for avEnableVSaaS()
    FILE *vsaas_info_file_ptr = fopen(gVsaasInfoFilePath, "w+");
    if (vsaas_info_file_ptr == NULL) {
        printf("%s fopen %s error!!\n", __func__, gVsaasInfoFilePath);
    } else {
        fwrite(vsaas_config, 1, strlen(vsaas_config), vsaas_info_file_ptr);
        fclose(vsaas_info_file_ptr);
        gVsaasConfigExist = true;
    }
}

static void VSaaSUpdateContractInfoHandle(const VSaaSContractInfo *contract_info)
{
    printf("Enter %s\n", __func__);
    printf("contract_type=%u\n", contract_info->contract_type);
    printf("event_recording_max_sec=%d\n", contract_info->event_recording_max_sec);
    printf("video_max_fps=%d\n", contract_info->video_max_fps);
    printf("recording_max_kbps=%d\n", contract_info->recording_max_kbps);
    printf("video_max_high=%d\n", contract_info->video_max_high);
    printf("video_max_width=%d\n", contract_info->video_max_width);
    
    gVsaasContractInfo.contract_type = contract_info->contract_type;
    gVsaasContractInfo.event_recording_max_sec = contract_info->event_recording_max_sec;
    gVsaasContractInfo.video_max_fps = contract_info->video_max_fps;
    gVsaasContractInfo.recording_max_kbps = contract_info->recording_max_kbps;
    gVsaasContractInfo.video_max_high = contract_info->video_max_high;
    gVsaasContractInfo.video_max_width = contract_info->video_max_width;

    //User should adopt the bit rate or resolution of record file according to the vsaas contract infomation.
}

//########################################################
//# Main function
//########################################################
int main(int argc, char *argv[])
{
    int ret = 0;
    FILE *fp = NULL;
    NebulaDeviceCtx *device_ctx = NULL;
    char *profile_buf = NULL;
    const char *profile_str = NULL;
    pthread_t nebula_login_thread_id;
    pthread_t iotc_login_thread_id;
    NebulaWakeUpData *sleep_data = NULL;
    unsigned int sleep_data_count = 0;
    char wakeup_pattern[MAX_WAKEUP_PATTERN_LENGTH] = {0};
    NebulaSocketProtocol nebula_protocol = NEBULA_PROTO_TCP;

    const char *license_key = "AQAAAHRU4THENnVzM+1AM2RI9FH9tF0SW+EvSIK0QsQpvak2EQM4aj3YpLu7cFlH/nxF6fOJm++AI1GEjgPYRBlJ2YOYIYOKoVbR7uKF+whZQdwheJpqSXEcGvoqHgvq+qANmvCTl5Kl+i3oHipeZOhx4wZPgCt9RL9DWUlwp53YGnMTanpelDVIg9Hmyedy4Z/FSQ7Ll22djvdeb7tMuHfvETAb";// getenv(ENV_LICENSE_KEY);
    if (license_key == NULL) {
        printf("Environment variable %s must be set!!\n", ENV_LICENSE_KEY);
        return -1;
    }
    ret = TUTK_SDK_Set_License_Key(license_key);
    printf("TUTK_SDK_Set_License_Key: %d\n", ret);
    if (ret < 0) {
        return -1;
    }

NEBULA_DEVICE_START:
    gProgressRun = true;
    gBindAbort = 0;

    /* Check command line options */
    ret = ParseInputOptions(argc, argv);
    if(ret < 0){
        printf("ParseInputOptions ret[%d]\n", ret);
        PrintUsage();
        return -1;
    }

    fp = fopen(gProfilePath, "r");
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    profile_buf = calloc(1, file_size + 1);
    fread(profile_buf, 1, file_size, fp);
    fclose(fp);
    profile_buf[file_size] = '\0';

    ret = loadDisposableParams(gPinCode, gSecretId, NULL);
    if(ret < 0) {
        printf("loadDisposableParams()=%d, %s exit...!!\n", ret, argv[0]);
        return -1;
    }

    PrintVersion();
    //Set log file path
    LogAttr log_attr;
    memset(&log_attr, 0, sizeof(log_attr));
    char log_path[] = "./device_log.txt";
    log_attr.path = log_path;

    ret = IOTC_Set_Log_Attr(log_attr);
    printf("IOTC_Set_Log_Attr ret[%d]\n", ret);

    //Initialize
    InitAVInfo();

    ret = CreateStreamoutThread();
    if (ret < 0) {
        printf("%s exit...!!\n", argv[0]);
        return -1;
    }

    // Nebula Initialize
    ret = Nebula_Initialize();
    printf("Nebula_Initialize ret[%d]\n", ret);
    if (ret != NEBULA_ER_NoERROR) {
        printf("Nebula_Device exit...!!\n");
        return -1;
    }

    IOTC_Set_Max_Session_Number(MAX_CLIENT_NUMBER);
    // IOTC Initialize
    ret = IOTC_Initialize2(0);
    printf("IOTC_Initialize() ret[%d]\n", ret);
    if (ret != IOTC_ER_NoERROR) {
        printf("%s exit...!!\n", argv[0]);
        return -1;
    }

    ret = Nebula_Json_Obj_Create_From_String(profile_buf, &gProfileJsonObj);
    if (ret != IOTC_ER_NoERROR) {
        printf("profile format error, %s exit...!!\n", argv[0]);
        return -1;
    }
    profile_str = Nebula_Json_Obj_To_String(gProfileJsonObj);
    ret = Nebula_Device_New(gUdid, gSecretId, profile_str, CommandHandle, IdentityHandle, SettingsChangeHandle, &device_ctx);
    printf("Nebula_Device_New ret[%d]\n", ret);
    if (ret != NEBULA_ER_NoERROR) {
        printf("%s exit...!!\n", argv[0]);
        return -1;
    }
    free(profile_buf);

    if(gEnableVsaas) {
        //read vsaas info file and enable vsaas
        char *vsaas_info = NULL;
        FILE *vsaas_info_file_ptr = NULL;
        vsaas_info_file_ptr = fopen(gVsaasInfoFilePath, "r");
        if (vsaas_info_file_ptr == NULL) {
            printf("Enable VSaaS without preload data!\n");
            printf("Device should get vsaas info from client,\n");
            printf("and save the vsaas info received from VsaasConfigChangedHandle\n");
            ret = avEnableVSaaSByNebula(device_ctx, NULL, VsaasConfigChangedHandle, VSaaSUpdateContractInfoHandle);
        } else {
            long file_size = 0;
            fseek(vsaas_info_file_ptr, 0, SEEK_END);
            file_size = ftell(vsaas_info_file_ptr);
            rewind(vsaas_info_file_ptr);

            vsaas_info = (char*)malloc(sizeof(char)*file_size);
            if(vsaas_info == NULL) {
                printf("memory alloc fail!\n");
                return -1;
            }

            fread(vsaas_info, 1, file_size, vsaas_info_file_ptr);

            printf("Enable VSaaS with preload data!\n");
            ret = avEnableVSaaSByNebula(device_ctx, vsaas_info, VsaasConfigChangedHandle, VSaaSUpdateContractInfoHandle);
            fclose(vsaas_info_file_ptr);
            free(vsaas_info);
            gVsaasConfigExist = true;
        }
        if (ret != AV_ER_NoERROR) {
            printf("avEnableVSaaSByNebula error [%d]\n", ret);
            return -1;
        }
        pthread_t fake_event_thread_id;
        if (pthread_create(&fake_event_thread_id, NULL, ThreadFakeVsaasEvent, NULL) < 0) {
            printf("create ThreadFakeVsaasEvent failed!\n");
        } else {
            pthread_detach(fake_event_thread_id);
        }
    }

    if (gEnablePushNotification) {
        FILE *fp = NULL;
        // get notification settings
        fp = fopen(gDefaultSettingsFilePath, "r");
        if (fp == NULL) {
            printf("%s not exist\n", gDefaultSettingsFilePath);
            return -1;
        }
        fseek(fp, 0, SEEK_END);
        long fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char *settings = calloc(1, fsize + 1);
        fread(settings, 1, fsize, fp);
        fclose(fp);

        ret = Nebula_Device_Load_Settings(device_ctx, settings);
        printf("Nebula_Device_Load_Settings[%d]\n", ret);
        if (ret == NEBULA_ER_NoERROR) {
            gSettingsExist = true;
        }
        free(settings);

        pthread_t fake_push_thread_id;
        if (pthread_create(&fake_push_thread_id, NULL, ThreadFakePushNotification, (void *)device_ctx) < 0) {
            printf("create ThreadFakePushNotification failed!\n");
        } else {
            pthread_detach(fake_push_thread_id);
        }
    }

    //AV Initialize
    avInitialize(MAX_CLIENT_NUMBER);

    if (pthread_create(&nebula_login_thread_id, NULL, ThreadNebulaLogin, device_ctx) < 0) {
        printf("create ThreadNebulaLogin failed!, %s exit...!!\n", argv[0]);
        return -1;
    }
    pthread_join(nebula_login_thread_id, NULL);

    if (pthread_create(&iotc_login_thread_id, NULL, ThreadIotcDeviceLogin, device_ctx) < 0) {
        printf("create ThreadIotcDeviceLogin failed!, %s exit...!!\n", argv[0]);
        gBindAbort = 1;
        return -1;
    }
    pthread_join(iotc_login_thread_id, NULL);

    ResetNoSessionTime();

    while(ShouldDeviceGoToSleep()==0) {
        ret = IOTC_Device_Listen_By_Nebula(device_ctx, 10000);
        printf("IOTC_Device_Listen_By_Nebula ret[%d]\n", ret);
        if (ret < 0) {
            PrintErrHandling(ret);
            UpdateNoSessionTime();
            continue;
        }

        if (ret >= 0) {
            ResetNoSessionTime();

            printf("Session[%d] connect!\n", ret);
            struct st_SInfoEx session_info;
            session_info.size = sizeof(session_info);
            if(IOTC_Session_Check_Ex(ret, &session_info) == IOTC_ER_NoERROR) {
                char *mode[3] = {"P2P", "RLY", "LAN"};
                if( isdigit( session_info.RemoteIP[0] ))
                    printf("Client is from[IP:%s, Port:%d] Mode[%s] VPG[%d:%d:%d] VER[%X] NAT[%d] AES[%d]\n", \
                    session_info.RemoteIP, session_info.RemotePort, mode[(int)session_info.Mode], session_info.VID, session_info.PID, session_info.GID, session_info.IOTCVersion, session_info.RemoteNatType, session_info.isSecure);

                int *sid = (int *)malloc(sizeof(int));
                *sid = ret;
                pthread_t thread_id;
                ret = pthread_create(&thread_id, NULL, &ThreadForAVServerStart, (void *)sid);
                if(ret < 0) {
                    printf("pthread_create ThreadForAVServerStart failed ret[%d]\n", ret);
                } else {
                    pthread_detach(thread_id);
                }
            }
        }
    }

    gProgressRun = false;
    gBindAbort = 1;
    Nebula_Json_Obj_Release(gProfileJsonObj);

    if(gEnableWakeUp){
        // Prepare wakeup data before deinitialize modules
        PrepareWakeupDataBeforeSleep(device_ctx, nebula_protocol, wakeup_pattern, &sleep_data, &sleep_data_count);
    }

    Nebula_Device_Delete(device_ctx);

    avDeInitialize();
    printf("avDeInitialize\n");
    IOTC_DeInitialize();
    printf("IOTC_DeInitialize\n");
    Nebula_DeInitialize();
    printf("Nebula_DeInitialize\n");
    DeInitAVInfo();

    if(gEnableWakeUp){
        // The code here is just for demo, users should implement the wakeup process according to your pratical use.
        // Wait for wakeup
        WaitForWakeupPatternWhenSleeping(nebula_protocol, wakeup_pattern, sleep_data, sleep_data_count);
        // After wakeup, restart the device process.
        goto NEBULA_DEVICE_START;
    }

    return 0;
}


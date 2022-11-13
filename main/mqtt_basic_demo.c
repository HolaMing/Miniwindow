/*
 * 这个例程适用于`Linux`这类支持pthread的POSIX设备, 它演示了用SDK配置MQTT参数并建立连接, 之后创建2个线程
 *
 * + 一个线程用于保活长连接
 * + 一个线程用于接收消息, 并在有消息到达时进入默认的数据回调, 在连接状态变化时进入事件回调
 *
 * 需要用户关注或修改的部分, 已经用 TODO 在注释中标明
 *
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/semphr.h>

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_ntp_api.h"
#include <lvgl.h>


extern SemaphoreHandle_t mutex_ntp;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint16_t msec;
} calender_time_t;

static calender_time_t ali_oclock;

void *mqtt_handle = NULL;
#if 1
// void *ntp_handle = NULL;

/* TODO: 替换为自己设备的三元组 */
char *product_key       = "hwvbWdRnXNi";
char *device_name       = "ESP32miniwindow01";
char *device_secret     = "6e22ad557d9b8d86462c6b642d85bea7";

/*
    TODO: 替换为自己实例的接入点

    对于企业实例, 或者2021年07月30日之后（含当日）开通的物联网平台服务下公共实例
    mqtt_host的格式为"${YourInstanceId}.mqtt.iothub.aliyuncs.com"
    其中${YourInstanceId}: 请替换为您企业/公共实例的Id

    对于2021年07月30日之前（不含当日）开通的物联网平台服务下公共实例
    需要将mqtt_host修改为: mqtt_host = "${YourProductKey}.iot-as-mqtt.${YourRegionId}.aliyuncs.com"
    其中, ${YourProductKey}：请替换为设备所属产品的ProductKey。可登录物联网平台控制台，在对应实例的设备详情页获取。
    ${YourRegionId}：请替换为您的物联网平台设备所在地域代码, 比如cn-shanghai等
    该情况下完整mqtt_host举例: a1TTmBPIChA.iot-as-mqtt.cn-shanghai.aliyuncs.com

    详情请见: https://help.aliyun.com/document_detail/147356.html
*/
char *mqtt_host = "iot-06z00es319h9r1u.mqtt.iothub.aliyuncs.com";

/* 位于portfiles/aiot_port文件夹下的系统适配函数集合 */
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/* 位于external/ali_ca_cert.c中的服务器证书 */
extern const char *ali_ca_cert;

// static pthread_t g_mqtt_process_thread;
// static pthread_t g_mqtt_recv_thread;
static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running = 0;
static StackType_t mqtt_process_proc_stack[1024];
static StaticTask_t g_mqtt_process_thread;
static StackType_t mqtt_recv_proc_stack[1024];
static StaticTask_t g_mqtt_recv_thread;


/* TODO: 如果要关闭日志, 就把这个函数实现为空, 如果要减少日志, 可根据code选择不打印
 *
 * 例如: [1577589489.033][LK-0317] mqtt_basic_demo&gb80sFmX7yX
 *
 * 上面这条日志的code就是0317(十六进制), code值的定义见core/aiot_state_api.h
 *
 */

/* 日志回调函数, SDK的日志会从这里输出 */
int32_t demo_state_logcb(int32_t code, char *message)
{
    printf("%s", message);
    return 0;
}

/* MQTT事件回调函数, 当网络连接/重连/断开时被触发, 事件定义见core/aiot_mqtt_api.h */
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
        /* SDK因为用户调用了aiot_mqtt_connect()接口, 与mqtt服务器建立连接已成功 */
        case AIOT_MQTTEVT_CONNECT: {
            printf("AIOT_MQTTEVT_CONNECT\r\n");
            /* TODO: 处理SDK建连成功, 不可以在这里调用耗时较长的阻塞函数 */
        } break;

        /* SDK因为网络状况被动断连后, 自动发起重连已成功 */
        case AIOT_MQTTEVT_RECONNECT: {
            printf("AIOT_MQTTEVT_RECONNECT\r\n");
            /* TODO: 处理SDK重连成功, 不可以在这里调用耗时较长的阻塞函数 */
        } break;

        /* SDK因为网络的状况而被动断开了连接, network是底层读写失败, heartbeat是没有按预期得到服务端心跳应答 */
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect")
                                                                                             : ("heartbeat disconnect");
            printf("AIOT_MQTTEVT_DISCONNECT: %s\r\n", cause);
            /* TODO: 处理SDK被动断连, 不可以在这里调用耗时较长的阻塞函数 */
        } break;

        default: {
        }
    }
}

/* MQTT默认消息处理回调, 当SDK从服务器收到MQTT消息时, 且无对应用户回调处理时被调用 */
void demo_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
            printf("heartbeat response\r\n");
            /* TODO: 处理服务器对心跳的回应, 一般不处理 */
        } break;

        case AIOT_MQTTRECV_SUB_ACK: {
            printf("suback, res: -0x%04X, packet id: %d, max qos: %d\r\n", -(uint16_t)packet->data.sub_ack.res,
                   packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
            /* TODO: 处理服务器对订阅请求的回应, 一般不处理 */
        } break;

        case AIOT_MQTTRECV_PUB: {
            printf("pub, qos: %d, topic: %.*s\r\n", packet->data.pub.qos, packet->data.pub.topic_len,
                   packet->data.pub.topic);
            printf("pub, payload: %.*s\r\n", (int)packet->data.pub.payload_len, packet->data.pub.payload);
            /* TODO: 处理服务器下发的业务报文 */
        } break;

        case AIOT_MQTTRECV_PUB_ACK: {
            printf("puback, packet id: %d\r\n", packet->data.pub_ack.packet_id);
            /* TODO: 处理服务器对QoS1上报消息的回应, 一般不处理 */
        } break;

        default: {
        }
    }
}

/* 执行aiot_mqtt_process的线程, 包含心跳发送和QoS1消息重发 */
void demo_mqtt_process_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_process_thread_running) {
        res = aiot_mqtt_process(args);
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        vTaskDelay(1000);
    }
    vTaskDelete(NULL);
}

/* 执行aiot_mqtt_recv的线程, 包含网络自动重连和从服务器收取MQTT消息 */
void demo_mqtt_recv_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_recv_thread_running) {
        res = aiot_mqtt_recv(args);
        if (res < STATE_SUCCESS) {
            if (res == STATE_USER_INPUT_EXEC_DISABLED) {
                break;
            }
            vTaskDelay(1000);
        }
    }
    vTaskDelete(NULL);
}
#endif

/* 事件处理回调,  */
void demo_ntp_event_handler(void *handle, const aiot_ntp_event_t *event, void *userdata)
{
    switch (event->type) {
        case AIOT_NTPEVT_INVALID_RESPONSE: {
            printf("AIOT_NTPEVT_INVALID_RESPONSE\n");
        } break;
        case AIOT_NTPEVT_INVALID_TIME_FORMAT: {
            printf("AIOT_NTPEVT_INVALID_TIME_FORMAT\n");
        } break;
        default: {
        }
    }
}

int32_t ali_mqtt_stop(void *mqtt_handle)
{
    int32_t res = STATE_SUCCESS;

    vTaskDelay(1000);
    g_mqtt_process_thread_running = 0;
    g_mqtt_recv_thread_running = 0;
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_disconnect failed: -0x%04X\r\n", -(uint16_t)res);
    }

    /* 销毁MQTT实例, 一般不会运行到这里 */
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_deinit failed: -0x%04X\r\n", -(uint16_t)res);
    }

    return 0;
}

void lv_oclock_display_init(lv_obj_t *t_date, lv_obj_t *t_time)
{
    lv_label_set_text_fmt(t_date, "%4u-%02u-%02u", ali_oclock.year, ali_oclock.month, ali_oclock.day);
    lv_label_set_text_fmt(t_time, "%02u:%02u:%02u", ali_oclock.hour, ali_oclock.min, ali_oclock.sec);

    lv_style_t style_text;
    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, lv_color_black());
    lv_obj_add_style(t_date, &style_text, 0);
    lv_obj_add_style(t_time, &style_text, 0);

    lv_obj_set_pos(t_date, 80, 10);
    lv_obj_set_pos(t_time, 140, 10);
}

void one_secend_handler()
{
    ali_oclock.sec++;
    if (ali_oclock.sec >= 60) {
        ali_oclock.min++;
    }
    if (ali_oclock.min >= 60) {
        ali_oclock.hour++;
    }
    if (ali_oclock.hour >= 24) {
        ali_oclock.hour = 0;
    }
    printf("one second passed\r\n");
}

void lv_oclock_display_update(lv_obj_t *t_date, lv_obj_t *t_time)
{
    lv_label_set_text_fmt(t_date, "%4u-%02u-%02u", ali_oclock.year, ali_oclock.month, ali_oclock.day);
    lv_label_set_text_fmt(t_time, "%02u:%02u:%02u", ali_oclock.hour, ali_oclock.min, ali_oclock.sec);
}


int get_ali_rtc_update(void *ntp_handle)
{
    int32_t res = STATE_SUCCESS;
    res = aiot_ntp_send_request(ntp_handle);
    if (res < STATE_SUCCESS) {
        aiot_ntp_deinit(&ntp_handle);
        return -1;
    }
    return 0;
}

void ali_ntp_display(void)
{
    lv_obj_t *text_date = lv_label_create(lv_scr_act());
    lv_obj_t *text_time = lv_label_create(lv_scr_act());
    // uint8_t tmp = ali_oclock.hour;
    // printf("data:%d\r\n", ali_oclock.year);
    lv_oclock_display_init(text_date, text_time);


    while (1) {
        vTaskDelay(500);
        lv_oclock_display_update(text_date, text_time);
        // if ((ali_oclock.hour <= 21) && ((ali_oclock.hour - tmp) >= 3)) {
        //     if (get_ali_rtc_update(ntp_handle) == 0) {
        //         tmp = ali_oclock.hour;
        //     }
        //     else {
        //         printf("get ali ntp update failed\r\n");
        //         vTaskDelete(NULL);
        // }
        // else {
        //     get_ali_rtc_update(ntp_handle);
        //     tmp = 0;
        // }
    }
}

void calender_timer_init(const aiot_ntp_recv_t *packet, calender_time_t *oclock)
{
    oclock->year = (uint16_t)packet->data.local_time.year;
    oclock->month = (uint8_t)packet->data.local_time.mon;
    oclock->day = (uint8_t)packet->data.local_time.day;
    oclock->hour = (uint8_t)packet->data.local_time.hour;
    oclock->min = (uint8_t)packet->data.local_time.min;
    oclock->sec = (uint8_t)packet->data.local_time.sec;
    oclock->msec = (uint16_t)packet->data.local_time.msec;
}

/* TODO: 数据处理回调, 当SDK从网络上收到ntp消息时被调用 */
void demo_ntp_recv_handler(void *handle, const aiot_ntp_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        /* TODO: 结构体 aiot_ntp_recv_t{} 中包含当前时区下, 年月日时分秒的数值, 可在这里把它们解析储存起来 */
        case AIOT_NTPRECV_LOCAL_TIME: {
            printf("local time: %llu, %02d/%02d/%02d-%02d:%02d:%02d:%d\r\n",
                   (long long unsigned int)packet->data.local_time.timestamp, (uint16_t)packet->data.local_time.year,
                   (uint16_t)packet->data.local_time.mon, (uint16_t)packet->data.local_time.day,
                   (uint16_t)packet->data.local_time.hour, (uint16_t)packet->data.local_time.min,
                   (uint16_t)packet->data.local_time.sec, (uint16_t)packet->data.local_time.msec);
        } break;

        default: {
        }
    }

    // calender_timer_init(packet, &ali_oclock);
    // static uint8_t i = 0;
    // if (i == 0) {
        // aos_post_event(EV_WIFI, ALI_NTP, 0);
        // TaskHandle_t ali_ntp_display_task;
        // xTaskCreate(ali_ntp_display, (char *)"ali_ntp", 1024, NULL, 15, &ali_ntp_display_task);
        // printf("ali_nep_display\r\n");
    //     i ++;
    // }
    // printf("[SYS] Memory left is %d Bytes\r\n", xPortGetFreeHeapSize());
    // display_oclock = calender_timer_upadate(&ali_oclock);
}

void *get_ali_rtc_init(void *ntp_handle)
{
    int32_t res = STATE_SUCCESS;
    int8_t time_zone = 8;
    /* 创建1个ntp客户端实例并内部初始化默认参数 */
    ntp_handle = aiot_ntp_init();
    if (ntp_handle == NULL) {
        printf("aiot_ntp_init failed\n");
        ali_mqtt_stop(mqtt_handle);
    }

    res = aiot_ntp_setopt(ntp_handle, AIOT_NTPOPT_MQTT_HANDLE, mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_ntp_setopt AIOT_NTPOPT_MQTT_HANDLE failed, res: -0x%04X\n", -(uint16_t)res);
        aiot_ntp_deinit(&ntp_handle);
        ali_mqtt_stop(mqtt_handle);
    }

    res = aiot_ntp_setopt(ntp_handle, AIOT_NTPOPT_TIME_ZONE, (int8_t *)&time_zone);
    if (res < STATE_SUCCESS) {
        printf("aiot_ntp_setopt AIOT_NTPOPT_TIME_ZONE failed, res: -0x%04X\n", -(uint16_t)res);
        aiot_ntp_deinit(&ntp_handle);
        ali_mqtt_stop(mqtt_handle);
    }

    /* TODO: NTP消息回应从云端到达设备时, 会进入此处设置的回调函数 */
    res = aiot_ntp_setopt(ntp_handle, AIOT_NTPOPT_RECV_HANDLER, (void *)demo_ntp_recv_handler);
    if (res < STATE_SUCCESS) {
        printf("aiot_ntp_setopt AIOT_NTPOPT_RECV_HANDLER failed, res: -0x%04X\n", -(uint16_t)res);
        aiot_ntp_deinit(&ntp_handle);
        ali_mqtt_stop(mqtt_handle);
    }

    res = aiot_ntp_setopt(ntp_handle, AIOT_NTPOPT_EVENT_HANDLER, (void *)demo_ntp_event_handler);
    if (res < STATE_SUCCESS) {
        printf("aiot_ntp_setopt AIOT_NTPOPT_EVENT_HANDLER failed, res: -0x%04X\n", -(uint16_t)res);
        aiot_ntp_deinit(&ntp_handle);
        ali_mqtt_stop(mqtt_handle);
    }

    /* 发送NTP查询请求给云平台 */
    res = aiot_ntp_send_request(ntp_handle);
    if (res < STATE_SUCCESS) {
        aiot_ntp_deinit(&ntp_handle);
        ali_mqtt_stop(mqtt_handle);
    }

    return ntp_handle;
}

#if 0
int ali_mqtt_connect(void)
{
    int32_t res = STATE_SUCCESS;
    uint16_t port = 443;             /* 无论设备是否使用TLS连接阿里云平台, 目的端口都是443 */
    aiot_sysdep_network_cred_t cred; /* 安全凭据结构体, 如果要用TLS, 这个结构体中配置CA证书等参数 */
    printf("ali\r\r\n");
    /* 配置SDK的底层依赖 */
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /* 配置SDK的日志输出 */
    aiot_state_set_logcb(demo_state_logcb);

    /* 创建SDK的安全凭据, 用于建立TLS连接 */
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA; /* 使用RSA证书校验MQTT服务端 */
    cred.max_tls_fragment = 16384;       /* 最大的分片长度为16K, 其它可选值还有4K, 2K, 1K, 0.5K */
    cred.sni_enabled = 1;                /* TLS建连时, 支持Server Name Indicator */
    cred.x509_server_cert = ali_ca_cert; /* 用来验证MQTT服务端的RSA根证书 */
    cred.x509_server_cert_len = strlen(ali_ca_cert); /* 用来验证MQTT服务端的RSA根证书长度 */

    /* 创建1个MQTT客户端实例并内部初始化默认参数 */
    mqtt_handle = aiot_mqtt_init();
    if (mqtt_handle == NULL) {
        ali_mqtt_stop(mqtt_handle);
        printf("aiot_mqtt_init failed\r\n");
        return -1;
    }

    /* TODO: 如果以下代码不被注释, 则例程会用TCP而不是TLS连接云平台 */
    /*
    {
        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
    }
    */

    /* 配置MQTT服务器地址 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_HOST, (void *)mqtt_host);
    /* 配置MQTT服务器端口 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&port);
    /* 配置设备productKey */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)product_key);
    /* 配置设备deviceName */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)device_name);
    /* 配置设备deviceSecret */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)device_secret);
    /* 配置网络连接的安全凭据, 上面已经创建好了 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    /* 配置MQTT默认消息接收回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)demo_mqtt_default_recv_handler);
    /* 配置MQTT事件回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);

    /* 与服务器建立MQTT连接 */
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        /* 尝试建立连接失败, 销毁MQTT实例, 回收资源 */
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_connect failed: -0x%04X\r\n\r\n", -(uint16_t)res);
        printf("please check variables like mqtt_host, produt_key, device_name, device_secret in demo\r\n");
        ali_mqtt_stop(mqtt_handle);
        return -1;
    }

#if 0
    /* MQTT 订阅topic功能示例, 请根据自己的业务需求进行使用 */
    {
        char *sub_topic = "/ext/ntp/hk2eXYpQx0a/${deviceName}/response";

        res = aiot_mqtt_sub(mqtt_handle, sub_topic, NULL, 1, NULL);
        if (res < 0) {
            printf("aiot_mqtt_sub failed, res: -0x%04X\r\n", -(uint16_t)res);
            return -1;
        }
    }

    /* MQTT 发布消息功能示例, 请根据自己的业务需求进行使用 */
    {
        char *pub_topic = "/ext/ntp/hk2eXYpQx0a/${deviceName}/request";
        // long long device_time = aos_now_ms();
        // printf("device time %lld\r\n", device_time);
        char pub_payload[32] = {0};
        // sprintf( pub_payload, "%lld", device_time);
        // printf("device time %s\r\n", pub_payload);

        res = aiot_mqtt_pub(mqtt_handle, pub_topic, (uint8_t *)pub_payload, (uint32_t)strlen(pub_payload), 0);
        if (res < 0) {
            printf("aiot_mqtt_sub failed, res: -0x%04X\r\n", -(uint16_t)res);
            return -1;
        }
    }
#endif

    /* 创建一个单独的线程, 专用于执行aiot_mqtt_process, 它会自动发送心跳保活, 以及重发QoS1的未应答报文 */
    g_mqtt_process_thread_running = 1;
    xTaskCreateStatic(demo_mqtt_process_thread, (char *)"demo_mqtt_process_thread", 1024, mqtt_handle, 15,
                      mqtt_process_proc_stack, &g_mqtt_process_thread);

    /* 创建一个单独的线程用于执行aiot_mqtt_recv, 它会循环收取服务器下发的MQTT消息, 并在断线时自动重连 */
    g_mqtt_recv_thread_running = 1;
    xTaskCreateStatic(demo_mqtt_recv_thread, (char *)"demo_mqtt_recv_thread", 2 * 1024, mqtt_handle, 15,
                      mqtt_recv_proc_stack, &g_mqtt_recv_thread);

    return 0;
}


int ali_ntp_connect(void)
{
    int32_t res = STATE_SUCCESS;
    /* 建立MQTT连接, 并开启保活线程和接收线程 */
    res = ali_mqtt_connect();
    if (res < 0) {
        printf("demo_mqtt_start failed\n");
        return -1;
    }
    printf("get time111111111111111111111\r\n");

    /* 创建1个ntp客户端实例并内部初始化默认参数 */
    ntp_handle = get_ali_rtc_init(ntp_handle);

    
    while (1) {
        printf("get time222222222222222222\r\n");
        vTaskDelay(3000);
        get_ali_rtc_update(ntp_handle);
    }
    // vTaskDelay(1000);
    // TaskHandle_t ali_ntp_display_task;
    // xTaskCreate(ali_ntp_display, (char *)"ali_ntp", 4096, NULL, 15, &ali_ntp_display_task);
    return 0;
}
#endif

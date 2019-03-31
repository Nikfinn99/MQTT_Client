#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <Client.h>
#include <ESP8266WiFi.h>
#include <SerialStream.h>

#ifndef MQTT_MAX_RETRY
#define MQTT_MAX_RETRY 10
#endif

#ifndef MQTT_RETRY_INTERVAL
#define MQTT_RETRY_INTERVAL 10000U
#endif

class MQTT_Client
{
public:
  enum Mode
  {
    SEND_ONLY,
    RECEIVE_ONLY,
    SEND_RECEIVE
  };

  typedef void (*mqtt_callback)(const String &, const String &, const String &);

  MQTT_Client(WiFiClient &client, const String server_ip, uint16_t server_port = 1883, const String user = "", const String pw = "", const String client_id = "");
  MQTT_Client(PubSubClient &client, const String server_ip, uint16_t server_port = 1883, const String user = "", const String pw = "", const String client_id = "");
  MQTT_Client(const String server_ip, uint16_t server_port = 1883, const String user = "", const String pw = "", const String client_id = "");
  MQTT_Client(const String server_ip, const String user = "", const String pw = "", const String client_id = "");

  void loop();
  bool publish(const String &sub_topic, const String &payload, bool retain = true);
  void setCallback(mqtt_callback callb);
  void setTopic(const String &topic);
  void setMode(Mode mode);
  void setLastWill(const String &will_topic, const String &will_message);
  void setInitPublish(const String &init_topic, const String &init_message);
  void enableDebug(bool deb);
  bool reconnect();
  bool connected();

  // debug functions from one to three parameters
  template <typename Generic>
  void mqttDebug(Generic text);
  template <typename Generic1, typename Generic2>
  void mqttDebug(Generic1 text1, Generic2 text2);
  template <typename Generic1, typename Generic2, typename Generic3>
  void mqttDebug(Generic1 text1, Generic2 text2, Generic3 text3);

protected:
  void callback_func(const char *p_topic, byte *p_payload, unsigned int p_length);
  void construct(const String &server_ip, const uint16_t server_port = 1883, const String &client_id = "");

  mqtt_callback m_callback = nullptr;

  WiFiClient *m_wificlient;
  PubSubClient *m_client;

  String m_server_ip;
  uint16_t m_server_port;
  String m_client_id;
  String m_pw;
  String m_user;

  String m_cmnd_topic;
  String m_stat_topic;
  String m_will_topic;
  String m_will_message;
  String m_init_topic;
  String m_init_message;

  unsigned long m_lastReconnectAttempt = 0;
  bool m_enable_debug = false;
  uint8_t m_retry_count = 0;

  Mode m_mode;
};

#endif // MQTT_CLIENT_H

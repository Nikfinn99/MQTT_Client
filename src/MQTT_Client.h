#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <Client.h>
#include <ESP8266WiFi.h>

class MQTT_Client
{
public:
  enum Mode
  {
    MODE_SEND_ONLY,
    MODE_RECEIVE_ONLY,
    MODE_SEND_RECEIVE
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

  template <typename Generic>
  void mqttDebug(Generic text);

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

  long m_lastReconnectAttempt = 0;
  bool m_enable_debug = false;

  Mode m_mode;
};

#endif // MQTT_CLIENT_H

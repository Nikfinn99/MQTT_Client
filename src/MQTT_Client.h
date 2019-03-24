#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <Client.h>
#include <ESP8266WiFi.h>
#include <string>

class MQTT_Client
{
public:
  enum Mode
  {
    MODE_SEND_ONLY,
    MODE_RECEIVE_ONLY,
    MODE_SEND_RECEIVE
  };

  typedef void (*mqtt_callback)(const std::string &, const std::string &, const std::string &);

  MQTT_Client(WiFiClient &client, const std::string server_ip, uint16_t server_port = 1883, const std::string user = "", const std::string pw = "", const std::string client_id = "");
  MQTT_Client(PubSubClient &client, const std::string server_ip, uint16_t server_port = 1883, const std::string user = "", const std::string pw = "", const std::string client_id = "");
  MQTT_Client(const std::string server_ip, uint16_t server_port = 1883, const std::string user = "", const std::string pw = "", const std::string client_id = "");
  MQTT_Client(const std::string server_ip, const std::string user = "", const std::string pw = "", const std::string client_id = "");

  void loop();
  bool publish(const std::string &sub_topic, const std::string &payload, bool retain = true);
  void setCallback(mqtt_callback callb);
  void setTopic(const std::string &topic);
  void setMode(Mode mode);
  void setLastWill(const std::string &will_topic, const std::string &will_message);
  void setInitPublish(const std::string &init_topic, const std::string &init_message);
  void enableDebug(bool deb);
  bool reconnect();
  bool connected();

  template <typename Generic>
  void mqttDebug(Generic text);

protected:
  void callback_func(const char *p_topic, byte *p_payload, unsigned int p_length);
  void construct(const std::string &server_ip, const uint16_t server_port = 1883, const std::string &client_id = "");

  mqtt_callback m_callback = nullptr;

  WiFiClient *m_wificlient;
  PubSubClient *m_client;

  std::string m_server_ip;
  uint16_t m_server_port;
  std::string m_client_id;
  std::string m_pw;
  std::string m_user;

  std::string m_cmnd_topic;
  std::string m_stat_topic;
  std::string m_will_topic;
  std::string m_will_message;
  std::string m_init_topic;
  std::string m_init_message;

  long m_lastReconnectAttempt = 0;
  bool m_enable_debug = false;

  Mode m_mode;
};

#endif // MQTT_CLIENT_H

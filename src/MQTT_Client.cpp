#include "MQTT_Client.h"
#include <IO.h>

//protected
void MQTT_Client::construct(const String &server_ip, const uint16_t server_port, const String &client_id)
{
  if (client_id.length() == 0)
  { //set client id if not specified by user
    char buf[20];
    itoa(ESP.getChipId(), buf, 16);
    m_client_id = "ESP-" + String(buf);
  }
  else
  {
    m_client_id = client_id; //assign user client id
  }
  m_server_ip = server_ip;
  m_server_port = server_port;

  m_client->setServer(m_server_ip.c_str(), m_server_port); //set server
  this->setMode(MODE_SEND_RECEIVE);                        //initial mode to send and receive
}

//public
MQTT_Client::MQTT_Client(const String server_ip, const String user, const String pw, const String client_id)
    : m_pw(pw), m_user(user)
{
  m_wificlient = new WiFiClient();
  m_client = new PubSubClient(*m_wificlient);
  this->construct(server_ip, 1883, client_id);
}

//public
MQTT_Client::MQTT_Client(const String server_ip, uint16_t server_port, const String user,
                         const String pw, const String client_id)
    : m_pw(pw), m_user(user)
{
  m_wificlient = new WiFiClient();
  m_client = new PubSubClient(*m_wificlient);
  this->construct(server_ip, server_port, client_id);
}

//public
MQTT_Client::MQTT_Client(PubSubClient &client, const String server_ip, uint16_t server_port,
                         const String user, const String pw, const String client_id)
    : m_pw(pw), m_user(user)
{
  m_wificlient = nullptr;
  m_client = &client;
  this->construct(server_ip, server_port, client_id);
}

//public
MQTT_Client::MQTT_Client(WiFiClient &client, const String server_ip, uint16_t server_port,
                         const String user, const String pw, const String client_id)
    : m_pw(pw), m_user(user)
{
  m_wificlient = &client;
  m_client = new PubSubClient(client);
  this->construct(server_ip, server_port, client_id);
}

//public
template <typename Generic>
void MQTT_Client::mqttDebug(Generic text)
{
  if (m_enable_debug)
  {
    Serial.print("*MQTT ");
    Serial.println(text);
  }
}

//public
void MQTT_Client::enableDebug(bool deb)
{
  m_enable_debug = deb;
}

//public
void MQTT_Client::loop()
{ //check connection and run PubSubClient loop

  if (!m_client->connected())
  { //reconnect non blocking
    long now = millis();
    if (now - m_lastReconnectAttempt > 5000 || m_lastReconnectAttempt == 0)
    {
      m_lastReconnectAttempt = now;
      // Attempt to reconnect
      if (this->reconnect())
      {
        m_lastReconnectAttempt = millis();
      }
    }
  }
  else
  {
    // Client connected
    m_client->loop();
  }
}

//public
bool MQTT_Client::connected()
{
  return m_client->connected();
}

//public
void MQTT_Client::setMode(Mode mode)
{
  m_mode = mode;
  switch (mode)
  {
  case MQTT_Client::MODE_SEND_ONLY: //send only mode -> unsubscribe from everything and delete callback
    m_client->unsubscribe(m_cmnd_topic.c_str());
    m_client->setCallback(nullptr);
    break;
  case MQTT_Client::MODE_RECEIVE_ONLY: //reveive only mode -> sub to cmnd topic
    m_client->subscribe(m_cmnd_topic.c_str());
    m_client->setCallback([=](char *top, uint8_t *pay, unsigned int len) { this->callback_func(top, pay, len); });
    break;
  case MQTT_Client::MODE_SEND_RECEIVE: //receive and send commands
    m_client->subscribe(m_cmnd_topic.c_str());
    m_client->setCallback([=](char *top, uint8_t *pay, unsigned int len) { this->callback_func(top, pay, len); });
    break;
  default: //error -> user hasn't used enum
    mqttDebug("Unsupported Mode!");
    break;
  }
}

//public
void MQTT_Client::setCallback(mqtt_callback callb)
{
  m_callback = callb;
}

//public
void MQTT_Client::setTopic(const String &topic)
{
  //prepend 'stat' or 'cmnd' to respective topics
  m_stat_topic = ("stat/" + topic);
  m_cmnd_topic = ("cmnd/" + topic + "/#"); //add wildcard to receive topic
}

//public
void MQTT_Client::setLastWill(const String &will_topic, const String &will_message)
{
  String top = m_stat_topic;
  if (will_topic.length() > 0)
  {
    top += "/" + will_topic;
  }
  m_will_topic = top;
  m_will_message = will_message;
}

//public
void MQTT_Client::setInitPublish(const String &init_topic, const String &init_message)
{
  String top = m_stat_topic;
  if (init_topic.length() > 0)
  {
    top += "/" + init_topic;
  }
  m_init_topic = top;
  m_init_message = init_message;
}

//public
bool MQTT_Client::publish(const String &sub_topic, const String &payload, bool retain)
{

  if (m_mode != MODE_RECEIVE_ONLY)
  { //don't allow publishing in receive only mode

    String top = m_stat_topic;
    if (sub_topic.length() > 0)
    {
      top += "/" + sub_topic;
    }
    mqttDebug((top + ":" + payload).c_str());
    return m_client->publish(top.c_str(), payload.c_str(), retain);
  }
  else
  {
    return false; //return false if in receive only mode
  }
}

//public
bool MQTT_Client::reconnect()
{
  mqttDebug("Attempting MQTT connection...");

  mqttDebug("params");
  mqttDebug(m_user.c_str());
  mqttDebug(m_pw.c_str());
  mqttDebug(m_server_ip.c_str());
  mqttDebug(m_server_port);
  mqttDebug(m_client_id.c_str());

  bool connected = false;
  if (m_will_topic.length() > 0 && m_will_message.length() > 0)
  { //set last will message on connect; QoS = 0, retain = true
    connected = m_client->connect(m_client_id.c_str(), m_user.c_str(), m_pw.c_str(), m_will_topic.c_str(), 0, true, m_will_message.c_str());
  }
  else
  {
    connected = m_client->connect(m_client_id.c_str(), m_user.c_str(), m_pw.c_str());
  }

  if (connected)
  { //reconnect and check if connected

    mqttDebug("connected");

    if (m_init_topic.length() > 0 && m_init_message.length() > 0)
    {
      mqttDebug("Sending initial message");
      m_client->publish(m_init_topic.c_str(), m_init_message.c_str()); //publish initial message
    }

    //resubscribe to both status and set
    m_client->subscribe(m_cmnd_topic.c_str());
    mqttDebug("Subscribing to");
    mqttDebug(m_cmnd_topic.c_str());
  }
  else
  {
    Serial.print("*MQTT error: failed, rc=");
    Serial.println(m_client->state());
    mqttDebug("debug: try again in 5 seconds");
  }
  return m_client->connected();
}

//protected
void MQTT_Client::callback_func(const char *p_topic, uint8_t *p_payload, unsigned int p_length)
{
  if (p_length > 0) //only if we received a payload
  {
    String topic(p_topic);

    // copy to temporary char array to add null termination character
    char *tmp_payload = new char[p_length + 1];
    for (uint8_t i = 0; i < p_length; i++)
    {
      tmp_payload[i] = p_payload[i];
    }
    tmp_payload[p_length] = '\0';

    String payload(tmp_payload);
    delete[] tmp_payload;

    String sub_topic(topic);
    int sub_index = topic.lastIndexOf('/');
    if (sub_index > 0)
    {
      sub_topic = sub_topic.substring(sub_index + 1); //extract target topic
    }
    String rem_topic = topic.substring(0, sub_index);                  //remaining topic minus target
    String dev_topic = rem_topic.substring(m_cmnd_topic.length() - 1); //extract device topic

    mqttDebug(topic);
    mqttDebug(payload);
    mqttDebug(dev_topic);
    mqttDebug(sub_topic);

    if (m_callback)
    { //callback set by user
      m_callback(dev_topic, sub_topic, payload);
    }
  }
}

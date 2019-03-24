#include "MQTT_Client.h"
#include <IO.h>

//protected
void MQTT_Client::construct(const std::string &server_ip, const uint16_t server_port, const std::string &client_id)
{
  if (client_id.empty())
  { //set client id if not specified by user
    char buf[20];
    itoa(ESP.getChipId(), buf, 16);
    m_client_id = "ESP-" + std::string(buf);
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
MQTT_Client::MQTT_Client(const std::string server_ip, const std::string user, const std::string pw, const std::string client_id)
    : m_pw(pw), m_user(user)
{
  m_wificlient = new WiFiClient();
  m_client = new PubSubClient(*m_wificlient);
  this->construct(server_ip, 1883, client_id);
}

//public
MQTT_Client::MQTT_Client(const std::string server_ip, uint16_t server_port, const std::string user,
                         const std::string pw, const std::string client_id)
    : m_pw(pw), m_user(user)
{
  m_wificlient = new WiFiClient();
  m_client = new PubSubClient(*m_wificlient);
  this->construct(server_ip, server_port, client_id);
}

//public
MQTT_Client::MQTT_Client(PubSubClient &client, const std::string server_ip, uint16_t server_port,
                         const std::string user, const std::string pw, const std::string client_id)
    : m_pw(pw), m_user(user)
{
  m_wificlient = nullptr;
  m_client = &client;
  this->construct(server_ip, server_port, client_id);
}

//public
MQTT_Client::MQTT_Client(WiFiClient &client, const std::string server_ip, uint16_t server_port,
                         const std::string user, const std::string pw, const std::string client_id)
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
void MQTT_Client::setTopic(const std::string &topic)
{
  //prepend 'stat' or 'cmnd' to respective topics
  m_stat_topic = ("stat/" + topic);
  m_cmnd_topic = ("cmnd/" + topic + "/#"); //add wildcard to receive topic
}

//public
void MQTT_Client::setLastWill(const std::string &will_topic, const std::string &will_message)
{
  std::string top = m_stat_topic;
  if (will_topic.length() > 0)
  {
    top += "/" + will_topic;
  }
  m_will_topic = top;
  m_will_message = will_message;
}

//public
void MQTT_Client::setInitPublish(const std::string &init_topic, const std::string &init_message)
{
  std::string top = m_stat_topic;
  if (init_topic.length() > 0)
  {
    top += "/" + init_topic;
  }
  m_init_topic = top;
  m_init_message = init_message;
}

//public
bool MQTT_Client::publish(const std::string &sub_topic, const std::string &payload, bool retain)
{

  if (m_mode != MODE_RECEIVE_ONLY)
  { //don't allow publishing in receive only mode

    std::string top = m_stat_topic;
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
  if (p_length > 0)//only if we received a payload
  {
    std::string topic(p_topic);
    std::string payload((const char *)p_payload, p_length);

    std::string sub_topic(topic);
    size_t sub_index = topic.find_last_of('/');
    if (sub_index != std::string::npos)
    {
      sub_topic = sub_topic.substr(sub_index + 1); //extract target topic
    }
    std::string rem_topic = topic.substr(0, sub_index);                  //remaining topic minus target
    std::string dev_topic = rem_topic.substr(m_cmnd_topic.length() - 1); //extract device topic

    mqttDebug(topic.c_str());
    mqttDebug(payload.c_str());
    mqttDebug(dev_topic.c_str());
    mqttDebug(sub_topic.c_str());

    if (m_callback)
    { //callback set by user
      m_callback(dev_topic, sub_topic, payload);
    }
  }
}

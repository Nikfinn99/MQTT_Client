#include "MQTT_Client.h"
#include <IO.h>

//protected
void MQTT_Client::construct(const String &server_ip, const uint16_t server_port, const String &client_id)
{
  // set client id if not specified by user
  if (client_id.length() == 0)
  {
    char buf[20];
    itoa(ESP.getChipId(), buf, 16);
    m_client_id = "ESP-" + String(buf);
  }
  else // assign user client id
  {
    m_client_id = client_id;
  }
  m_server_ip = server_ip;
  m_server_port = server_port;

  // set server
  m_client->setServer(m_server_ip.c_str(), m_server_port);

  // set mode to send and receive
  this->setMode(SEND_RECEIVE);
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
    Serial << "*MQTT " << text << endl;
  }
}

//public
template <typename Generic1, typename Generic2>
void MQTT_Client::mqttDebug(Generic1 text1, Generic2 text2)
{
  if (m_enable_debug)
  {
    Serial << "*MQTT " << text1 << space << text2 << endl;
  }
}

//public
template <typename Generic1, typename Generic2, typename Generic3>
void MQTT_Client::mqttDebug(Generic1 text1, Generic2 text2, Generic3 text3)
{
  if (m_enable_debug)
  {
    Serial << "*MQTT " << text1 << space << text2 << space << text3 << endl;
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
  {
    //reconnect non blocking
    if (millis() > m_lastReconnectAttempt + MQTT_RETRY_INTERVAL || m_lastReconnectAttempt == 0)
    {
      // increase retry counter
      m_retry_count++;

      // Attempt to reconnect
      if (!this->reconnect())
      {
        // restart if max retries has been reached
        if (m_retry_count >= MQTT_MAX_RETRY)
        {
          ESP.restart();
          while (1)
          {
            delay(500);
            DEBUG_LED.toggle();
          }
        }
      }

      // update reconnect timestamp
      m_lastReconnectAttempt = millis();
    }
  }
  else
  {
    // Client connected
    m_client->loop();
    m_retry_count = 0;
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
  case MQTT_Client::SEND_ONLY: //send only mode -> unsubscribe from everything and delete callback
    m_client->unsubscribe(m_cmnd_topic.c_str());
    m_client->setCallback(nullptr);
    break;
  case MQTT_Client::RECEIVE_ONLY: //receive only mode -> sub to cmnd topic
  case MQTT_Client::SEND_RECEIVE: //receive and send commands
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

  if (m_mode != RECEIVE_ONLY)
  { //don't allow publishing in receive only mode

    String top = m_stat_topic;
    if (sub_topic.length() > 0)
    {
      top += "/" + sub_topic;
    }
    mqttDebug(top + ":" + payload);
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
  mqttDebug("user", m_user);
  mqttDebug("pw", m_pw);
  mqttDebug("ip", m_server_ip);
  mqttDebug("port", m_server_port);
  mqttDebug("client_id", m_client_id);

  if (m_will_topic.length() > 0 && m_will_message.length() > 0)
  { //set last will message on connect; QoS = 0, retain = true
    m_client->connect(m_client_id.c_str(), m_user.c_str(), m_pw.c_str(), m_will_topic.c_str(), 0, true, m_will_message.c_str());
  }
  else
  {
    m_client->connect(m_client_id.c_str(), m_user.c_str(), m_pw.c_str());
  }

  if (m_client->connected())
  {
    //connection sucessfull

    mqttDebug("connected");

    //resubscribe to cmnd topic
    m_client->subscribe(m_cmnd_topic.c_str());
    mqttDebug("Subscribing to", m_cmnd_topic);

    //publish initial message
    if (m_init_topic.length() > 0 && m_init_message.length() > 0)
    {
      mqttDebug("Sending initial message");
      m_client->publish(m_init_topic.c_str(), m_init_message.c_str());
    }
  }
  else
  {
    mqttDebug("error: failed, rc=", m_client->state());
    mqttDebug("trying again in", MQTT_RETRY_INTERVAL / 1000, "seconds");
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

    String action_topic(topic);
    int sub_index = topic.lastIndexOf('/');
    if (sub_index > 0)
    {
      action_topic = action_topic.substring(sub_index + 1); //extract action topic
    }
    String rem_topic = topic.substring(0, sub_index);                     //remaining topic minus action
    String device_topic = rem_topic.substring(m_cmnd_topic.length() - 1); //extract device topic

    mqttDebug("Message received:");
    mqttDebug("topic", topic);
    mqttDebug("payload", payload);
    mqttDebug("device", device_topic);
    mqttDebug("action", action_topic);

    if (m_callback)
    { //callback set by user
      m_callback(device_topic, action_topic, payload);
    }
  }
}

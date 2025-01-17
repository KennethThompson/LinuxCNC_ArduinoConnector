#ifndef CONNECTION_H_
#define CONNECTION_H_
#pragma once
#include "Protocol.h"

#include "CRC8.h"
#include "CRC.h"

enum ConnectionState
{
  CS_DISCONNECTED = 0,
  CS_CONNECTING,
  CS_CONNECTED,
  CS_RECONNECTED,
  CS_DISCONNECTING,
  CS_CONNECTION_TIMEOUT,
  CS_ERROR
};
CRC8 _crc;
class ConnectionBase {
public:
  ConnectionBase(uint16_t retryPeriod, uint64_t& fm) : _retryPeriod(retryPeriod), _featureMap(fm)
  {

  }

  virtual ~ConnectionBase(){}

  #ifdef DEBUG
  virtual void SendDebugMessage(String& message)
  {
    _sendDebugMessage(message);
  }
  #endif

  int& GetState()
  {
    return _myState;
  }

  virtual void SendPinStatusMessage(char sig, int pin, int state)
  {
    String status = String(sig);
    status += String(pin);
    status += ":";
    status += String(state);
    protocol::pm.status = status;
    protocol::pm.status += " ";
    _sendPinStatusMessage();
  }

  virtual void SendPinStatusMessage(char sig, int pin, double state, int decimals)
  {
    String status = String(sig);
    status += String(pin);
    status += ":";
    status += String(state, decimals);
    protocol::pm.status = status;
    protocol::pm.status += " ";
    _sendPinStatusMessage();
  }


  void DoWork()
  {
    if( _initialized == false)
    {
      if ( this->_onInit() )
      {
        _initialized = true;
        
      }
      else
      {
        delay(1000);
        return;
      }
    }
    /* TODO: Move this code out to appropriate connection-deriving classes.
    #if DHCP == 1
      if(millis() > this->_dhcpMaintTimer + _retryPeriod)
      {
        do_dhcp_maint();
        this->_dhcpMaintTimer = millis();
      }
    #endif
    */

    switch(_myState)
    {
      case CS_CONNECTION_TIMEOUT:
      case CS_DISCONNECTED:
      {
        this->_setState(CS_CONNECTING);
        this->_sendHandshakeMessage();
        _resendTimer = millis();
        _receiveTimer = millis(); 
        _handshakeReceived = 0;
        _heartbeatReceived = 0;
        _commandReceived = 0;
        /*
        #ifdef DEBUG
          Serial.print("ARDUINO DEBUG: UDP disconnected, retrying connection to ");
          Serial.print(this->IpAddress2String(this->_serverIP));
          Serial.print(":");
          Serial.print(this->_txPort);
          Serial.print(" in ");
          Serial.print(_retryPeriod/1000);
          Serial.println(" seconds...");
        #endif
        */
        break;
      }
      case CS_CONNECTING:
      {
        //Serial.println(_retryPeriod);
        if(_getHandshakeReceived())
        {
          this->_setState(CS_CONNECTED);
          _resendTimer = millis();
          _receiveTimer = millis();
          return;
        }
        else if(millis() > this->_resendTimer + _retryPeriod)
        {
          // TIMED OUT
          this->_setState(CS_ERROR);
          return;
        }
        break;
      }
      case CS_RECONNECTED:
         this->_setState(CS_CONNECTED);
         return;
         //break;
      case CS_CONNECTED:
      {
        if(_getHandshakeReceived())
        {
          this->_setState(CS_RECONNECTED);
          _resendTimer = millis();
          _receiveTimer = millis();
          return;
        }
        else if ( millis() > this->_resendTimer + (_retryPeriod) )
        {
          _sendHeartbeatMessage();
          _resendTimer = millis();
        }
        else if ( _getHeartbeatReceived() ) // Heartbeat received, reset timeout
        {
          //String message = "Hello - I am debug from the Arduino!";
          //SendDebugMessage(message);
          _receiveTimer = millis();
        }
        else if ( millis() > this->_receiveTimer + (_retryPeriod*2) )
        {
          this->_setState(CS_CONNECTION_TIMEOUT);
          return;
          //sendHeartbeatMessage();
          //this->_resendTimer = millis();
        }
        /*
          int packetSize = _udpClient.parsePacket();
          if (packetSize) {
            Serial.print("Received packet of size ");
            Serial.println(packetSize);
            Serial.print("From ");
            IPAddress remote = _udpClient.remoteIP();
            for (int i=0; i < 4; i++) {
              Serial.print(remote[i], DEC);
              if (i < 3) {
                Serial.print(".");
              }
            }
            Serial.print(", port ");
            Serial.println(_udpClient.remotePort());
          }
          */
          break;
      }
      case CS_ERROR:
      {
        this->_setState(CS_DISCONNECTED);
        return;
        //return 0;
      }
    }
    this->_onDoWork();
        // must be called to trigger callback and publish data
    //MsgPacketizer::update();
  }
  
  #ifdef DEBUG

  String IpAddress2String(const IPAddress& ipAddress)
  {
    return String(ipAddress[0]) + String(".") +\
    String(ipAddress[1]) + String(".") +\
    String(ipAddress[2]) + String(".") +\
    String(ipAddress[3]); 
  }
  String stateToString(int& state)
  {
    switch(state)
    {
      case CS_DISCONNECTED:
        return String("CS_DISCONNECTED");
      case CS_CONNECTING:
        return String("CS_CONNECTING");
      case CS_CONNECTED:
        return String("CS_CONNECTED");
      case CS_RECONNECTED:
        return String("CS_RECONNECTED");
      case CS_DISCONNECTING:
        return String("CS_DISCONNECTING");
      case CS_ERROR:
        return String("CS_ERROR");
      case CS_CONNECTION_TIMEOUT:
        return String("CS_CONNECTION_TIMEOUT");
      default:
        return String("CS_UNKNOWN_STATE");
    }
  }
  #endif

  uint8_t CommandReceived()
  {
    return _commandReceived;
  }

  const protocol::CommandMessage& GetReceivedCommand()
  {
    uint8_t r = _commandReceived;
    if (_commandReceived)
      _commandReceived = 0; // Clear flag.  Crude, but works.
    return protocol::cm;
  }

protected:
  virtual uint8_t _onInit();
  virtual void _onConnect();
  virtual void _onDisconnect();
  virtual void _onError();
  virtual void _onDoWork();


  virtual void _sendHandshakeMessage();

  virtual void _sendHeartbeatMessage();

  virtual void _sendPinStatusMessage();
  #ifdef DEBUG
  virtual void _sendDebugMessage(String& message);
  #endif

  void _onHandshakeMessage(const protocol::HandshakeMessage& n)
  {
      #ifdef DEBUG_PROTOCOL_VERBOSE
      Serial.println("ARDUINO DEBUG: ---- RX HANDSHAKE MESSAGE DUMP ----");
      Serial.print("ARDUINO DEBUG: Protocol Version: 0x");
      Serial.println(n.protocolVersion, HEX);
      Serial.print("ARDUINO DEBUG: Feature Map: 0x");
      Serial.println(n.featureMap, HEX);
      Serial.print("ARDUINO DEBUG: Board Index: ");
      Serial.println(n.boardIndex);
      Serial.println("ARDUINO DEBUG: ---- RX END HANDSHAKE MESSAGE DUMP ----");
      #endif
      _handshakeReceived = 1;
  }

  void _onHeartbeatMessage(const protocol::HeartbeatMessage& n)
  {
      #ifdef DEBUG_PROTOCOL_VERBOSE
      Serial.println("ARDUINO DEBUG: ---- RX HEARTBEAT MESSAGE DUMP ----");
      Serial.print("ARDUINO DEBUG: Board Index: ");
      Serial.println(n.boardIndex);
      Serial.println("ARDUINO DEBUG: ---- RX END HEARTBEAT MESSAGE DUMP ----");
      #endif
      _heartbeatReceived = 1;
  }

  void _onCommandMessage(const protocol::CommandMessage& n)
  {
      #ifdef DEBUG_PROTOCOL_VERBOSE
      Serial.println("ARDUINO DEBUG: ---- RX COMMAND MESSAGE DUMP ----");
      Serial.print("ARDUINO DEBUG: Command: ");
      Serial.println(n.cmd);
      Serial.print("ARDUINO DEBUG: Board Index: ");
      Serial.println(n.boardIndex);
      Serial.println("ARDUINO DEBUG: ---- RX END COMMAND MESSAGE DUMP ----");
      #endif
      protocol::cm.cmd = n.cmd;
      protocol::cm.boardIndex = n.boardIndex-1;
      _commandReceived = 1;
  }

  void _setState(int new_state)
  {
    #ifdef DEBUG
      Serial.flush();
      Serial.print("ARDUINO DEBUG: Connection transitioning from current state of [");
      Serial.print(this->stateToString(this->_myState));
      Serial.print("] to [");
      Serial.print(this->stateToString(new_state));
      Serial.println("]");
      Serial.flush();
    #endif

    this->_myState = new_state;
    if (new_state == CS_DISCONNECTED)
    {
      _onDisconnect();
    }
    else if (new_state == CS_CONNECTED)
    {
      _onConnect();
    }
  }

  protocol::HandshakeMessage& _getHandshakeMessage()
  {
    protocol::hm.featureMap = this->_featureMap;
    protocol::hm.timeout = _retryPeriod * 2;
    #ifdef DEBUG_PROTOCOL_VERBOSE   
      Serial.println("ARDUINO DEBUG: ---- TX HANDSHAKE MESSAGE DUMP ----");
      Serial.print("ARDUINO DEBUG: Protocol Version: 0x");
      Serial.println(protocol::hm.protocolVersion, HEX);
      Serial.print("ARDUINO DEBUG: Feature Map: 0x");
      Serial.println(protocol::hm.featureMap, HEX);
      Serial.print("ARDUINO DEBUG: Timeout: ");
      Serial.println(protocol::hm.timeout);
      Serial.print("ARDUINO DEBUG: Board Index: ");
      Serial.println(protocol::hm.boardIndex);
      Serial.println("ARDUINO DEBUG: ---- TX END HANDSHAKE MESSAGE DUMP ----");
    #endif
    return protocol::hm;
  }
/*
  size_t _packetizeMessage(MsgPack::Packer& packer, uint8_t type, byte* outBuffer)
  {
    uint8_t psize = packer.size();
    _crc.add((uint8_t*)packer.data(), psize);
    uint8_t crc = _crc.calc();
    _crc.restart();
    uint8_t t = type;
    uint8_t eot = 0x00;

    
    memcpy(outBuffer, (void*)&psize, sizeof(byte));
    memcpy(((byte*)outBuffer)+1, (void*)&t, sizeof(byte));
    memcpy(((byte*)outBuffer)+2, (void*)packer.data(), psize);
    memcpy(((byte*)outBuffer)+2+psize, (void*)&crc, sizeof(byte));
    memcpy(((byte*)outBuffer)+2+psize+1, (void*)&eot, sizeof(byte));

    return 4 + psize;
  }
  */
  /*
  size_t _getHandshakeMessagePacked(byte* outBuffer)
  {
    MsgPack::Packer packer;
    protocol::hm.featureMap = 0x100003;//this->_featureMap;
    protocol::hm.timeout = _retryPeriod * 2;
    packer.serialize(protocol::hm);
    uint8_t size = packer.size();
    size_t packetsize = _packetizeMessage(packer, protocol::MT_HANDSHAKE, outBuffer);
    #ifdef DEBUG_PROTOCOL_VERBOSE
      Serial.print("DEBUG: ---------START TX DUMP FOR MT_HANDSHAKE PAYLOAD: ");
      for( int x = 0; x < packer.size(); x++ )
      {
        Serial.print("[0x");
        Serial.print(packer.data()[x], HEX);
        Serial.print("]");
      }     
      Serial.println(" --------END TX DUMP FOR MT_HANDSHAKE");
    #endif

    #ifdef DEBUG_PROTOCOL_VERBOSE
      Serial.print("DEBUG: START TX DUMP FOR MT_HANDSHAKE");
      for( int x = 0; x < packetsize; x++ )
      {
        Serial.print("[0x");
        Serial.print(outBuffer[x], HEX);
        Serial.print("]");
      }     
      Serial.println("END TX DUMP FOR MT_HANDSHAKE");
    #endif
    return packetsize;
  }
  */
  /*
  size_t _getHeartbeatMessagePacked(byte* outBuffer)
  {
    MsgPack::Packer packer;
    packer.serialize(protocol::hb);
    uint8_t size = packer.size();
    size_t packetsize = _packetizeMessage(packer, protocol::MT_HEARTBEAT, outBuffer);

    #ifdef DEBUG_PROTOCOL_VERBOSE
      Serial.print("DEBUG: START TX DUMP FOR MT_HEARTBEAT");
      for( int x = 0; x < packetsize; x++ )
      {
        Serial.print("[0x");
        Serial.print(_txBuffer[x], HEX);
        Serial.print("]");
      }     
      Serial.println("END TX DUMP FOR MT_HEARTBEAT");
    #endif
    return packetsize;
  }
  */
  protocol::HeartbeatMessage& _getHeartbeatMessage()
  {
    #ifdef DEBUG_PROTOCOL_VERBOSE
      Serial.println("ARDUINO DEBUG: ---- TX HEARTBEAT MESSAGE DUMP ----");
      Serial.print("ARDUINO DEBUG: Board Index: ");
      Serial.println(protocol::hb.boardIndex);
      Serial.println("ARDUINO DEBUG: ---- TX END HEARTBEAT MESSAGE DUMP ----");
    #endif
    return protocol::hb;
  }
  /*
  size_t _getPinStatusMessagePacked(byte* outBuffer)
  {
    MsgPack::Packer packer;
    packer.serialize(protocol::pm);
    uint8_t size = packer.size();
    size_t packetsize = _packetizeMessage(packer, protocol::MT_PINSTATUS, outBuffer);

    #ifdef DEBUG_PROTOCOL_VERBOSE
      Serial.print("DEBUG: START TX DUMP FOR MT_PINSTATUS");
      for( int x = 0; x < packetsize; x++ )
      {
        Serial.print("[0x");
        Serial.print(_txBuffer[x], HEX);
        Serial.print("]");
      }     
      Serial.println("END TX DUMP FOR MT_PINSTATUS");
    #endif
    return packetsize;
  }
  */
  protocol::PinStatusMessage& _getPinStatusMessage()
  {
    #ifdef DEBUG_PROTOCOL_VERBOSE
      Serial.println("ARDUINO DEBUG: ---- TX PINSTATUS MESSAGE DUMP ----");
      Serial.print("ARDUINO DEBUG: STATUS: ");
      Serial.println(protocol::pm.status);      
      Serial.print("ARDUINO DEBUG: Board Index: ");
      Serial.println(protocol::pm.boardIndex);
      Serial.println("ARDUINO DEBUG: ---- TX END PINSTATUS MESSAGE DUMP ----");
    #endif
    return protocol::pm;
  }

  #ifdef DEBUG
  /*
  size_t _getDebugMessagePacked(byte* outBuffer, String& message)
  {
    MsgPack::Packer packer;
    protocol::dm.message = message;
    packer.serialize(protocol::dm);
    uint8_t size = packer.size();
    size_t packetsize = _packetizeMessage(packer, protocol::MT_DEBUG, outBuffer);

    #ifdef DEBUG_PROTOCOL_VERBOSE
      Serial.print("DEBUG: START TX DUMP FOR MT_DEBUGMESSAGE");
      for( int x = 0; x < packetsize; x++ )
      {
        Serial.print("[0x");
        Serial.print(_txBuffer[x], HEX);
        Serial.print("]");
      }     
      Serial.println("END TX DUMP FOR MT_DEBUGMESSAGE");
    #endif
    return packetsize;
  }
  */
  protocol::DebugMessage& _getDebugMessage(String& message)
  {
    protocol::dm.message = message;
    // No need to wrap in DEBUG define as the entire method is only compiled in when DEBUG is defined
    Serial.println("ARDUINO DEBUG: ---- TX DEBUG MESSAGE DUMP ----");
    Serial.print("ARDUINO DEBUG: Board Index: ");
    Serial.println(protocol::dm.boardIndex);
    Serial.print("ARDUINO DEBUG: Message: ");
    Serial.println(protocol::dm.message);
    Serial.println("ARDUINO DEBUG: ---- TX END DEBUG MESSAGE DUMP ----");
    return protocol::dm;
  }
  #endif
  

  uint8_t _getHandshakeReceived()
  {
    uint8_t r = _handshakeReceived;
    if (_handshakeReceived)
      _handshakeReceived = 0;
    return r;
  }
  uint8_t _getHeartbeatReceived()
  {
    uint8_t r = _heartbeatReceived;
    if (_heartbeatReceived)
      _heartbeatReceived = 0;
    return r;
  }

  
  uint16_t _retryPeriod = 0;
  uint32_t _resendTimer = 0;
  uint32_t _receiveTimer = 0;
  uint8_t _initialized = false;
  uint64_t _featureMap = 1;
  int _myState = CS_DISCONNECTED;
  char _rxBuffer[RX_BUFFER_SIZE];
  byte _txBuffer[RX_BUFFER_SIZE];
  // Flip-flops used to signal the presence of received message types
  uint8_t _handshakeReceived = 0;
  uint8_t _heartbeatReceived = 0;
  uint8_t _commandReceived = 0;

 

};
#endif
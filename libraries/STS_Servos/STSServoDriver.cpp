#include "STSServoDriver.h"

STSServoDriver::STSServoDriver()
{
}


STSServoDriver::~STSServoDriver()
{
}


bool STSServoDriver::init(PinName dirPin, BufferedSerial *serialPort,long const& baudRate, chrono::milliseconds timeOut)
{
    // Open port
    port_ = serialPort;
    port_->set_baud(baudRate);
    port_->set_blocking(false);
    timeOut_ = timeOut;
    dirPin_.~DigitalOut();
    new (&dirPin_) DigitalOut(dirPin);

    // Test that at least one servo is present.
    for (uint8_t i = 0; i < 0xFE; i++)
    {
        if (ping(i))
        {
            return true;
        }
    }
    return false;
}


bool STSServoDriver::ping(uint8_t const& servoId)
{
    flush();
    uint8_t response[1] = {0xFF};
    int send = sendMessage(servoId,
                           STS::PING,
                           0,
                           response);
    // Failed to send
    if (send != 6)
        return false;
    // Read response
    int rd = recieveMessage(servoId, 1, response);
    if (rd < 0)
        return false;
    return response[0] == 0x00;
}


bool STSServoDriver::setId(uint8_t const& oldServoId, uint8_t const& newServoId)
{
    if (oldServoId >= 0xFE || newServoId >= 0xFE)
        return false;
    if (ping(newServoId))
        return false; // address taken
    // Unlock EEPROM
    if (!writeRegister(oldServoId, STS::WRITE_LOCK, 0))
        return false;
    // Write new ID
    if (!writeRegister(oldServoId, STS::ID, newServoId))
        return false;
    // Lock EEPROM
    if (!writeRegister(newServoId, STS::WRITE_LOCK, 1))
      return false;
    return ping(newServoId);
}


int STSServoDriver::getCurrentPosition(uint8_t const& servoId)
{
    int16_t pos = readTwouint8_tsRegister(servoId, STS::CURRENT_POSITION);
    return pos;
}


int STSServoDriver::getCurrentSpeed(uint8_t const& servoId)
{
    int16_t vel = readTwouint8_tsRegister(servoId, STS::CURRENT_SPEED);
    return vel;
}


int STSServoDriver::getCurrentTemperature(uint8_t const& servoId)
{
    return readTwouint8_tsRegister(servoId, STS::CURRENT_TEMPERATURE);
}


int STSServoDriver::getCurrentCurrent(uint8_t const& servoId)
{
    int16_t current = readTwouint8_tsRegister(servoId, STS::CURRENT_CURRENT);
    return current * 0.0065;
}

bool STSServoDriver::isMoving(uint8_t const& servoId)
{
    uint8_t const result = readRegister(servoId, STS::MOVING_STATUS);
    return result > 0;
}


bool STSServoDriver::setTargetPosition(uint8_t const& servoId, int const& position, bool const& asynchronous)
{
    return writeTwouint8_tsRegister(servoId, STS::TARGET_POSITION, position, asynchronous);
}


bool STSServoDriver::setTargetVelocity(uint8_t const& servoId, int const& velocity, bool const& asynchronous)
{
    return writeTwouint8_tsRegister(servoId, STS::RUNNING_SPEED, velocity, asynchronous);
}


bool STSServoDriver::trigerAction()
{
    uint8_t noParam = 0;
    int send = sendMessage(0xFE, STS::ACTION, 0, &noParam);
    return send == 6;
}

int STSServoDriver::sendMessage(uint8_t const& servoId,
                                uint8_t const& commandID,
                                uint8_t const& paramLength,
                                uint8_t *parameters)
{
    uint8_t message[6 + paramLength];
    uint8_t checksum = servoId + paramLength + 2 + commandID;
    message[0] = 0xFF;
    message[1] = 0xFF;
    message[2] = servoId;
    message[3] = paramLength + 2;
    message[4] = commandID;
    for (int i = 0; i < paramLength; i++)
    {
        message[5 + i] = parameters[i];
        checksum += parameters[i];
    }
    message[5 + paramLength] = ~checksum;

    dirPin_ = 1;
    int ret = port_->write(message, 6 + paramLength);
    dirPin_ = 0;

    // Give time for the message to be processed.
    ThisThread::sleep_for(timeOut_);
    return ret;
}


bool STSServoDriver::writeRegisters(uint8_t const& servoId,
                                    uint8_t const& startRegister,
                                    uint8_t const& writeLength,
                                    uint8_t const *parameters,
                                    bool const& asynchronous)
{
    uint8_t param[writeLength + 1];
    param[0] = startRegister;
    for (int i = 0; i < writeLength; i++)
        param[i + 1] = parameters[i];
    int rc =  sendMessage(servoId,
                          asynchronous ? STS::REGWRITE : STS::WRITE,
                          writeLength + 1,
                          param);
    return rc == writeLength + 7;
}


bool STSServoDriver::writeRegister(uint8_t const& servoId,
                                   uint8_t const& registerId,
                                   uint8_t const& value,
                                   bool const& asynchronous)
{
    return writeRegisters(servoId, registerId, 1, &value, asynchronous);
}


uint8_t STSServoDriver::readRegister(uint8_t const& servoId, uint8_t const& registerId)
{
    uint8_t result = 0;
    int rc = readRegisters(servoId, registerId, 1, &result);
    if (rc < 0)
        return 0;
    return result;
}


bool STSServoDriver::writeTwouint8_tsRegister(uint8_t const& servoId,
                                           uint8_t const& registerId,
                                           int16_t const& value,
                                           bool const& asynchronous)
{
    uint8_t params[2] = { static_cast<unsigned char>(value & 0xFF), 
                               static_cast<unsigned char>((value >> 8) & 0xFF) };
    return writeRegisters(servoId, registerId, 2, params, asynchronous);
}


int16_t STSServoDriver::readTwouint8_tsRegister(uint8_t const& servoId, uint8_t const& registerId)
{
    uint8_t result[2] = {0, 0};
    int rc = readRegisters(servoId, registerId, 2, result);
    if (rc < 0)
        return 0;
    return result[0] + (result[1] << 8);
}


int STSServoDriver::readRegisters(uint8_t const& servoId,
                                  uint8_t const& startRegister,
                                  uint8_t const& readLength,
                                  uint8_t *outputBuffer)
{
    uint8_t readParam[2] = {startRegister, readLength};
    flush();
    int send = sendMessage(servoId, STS::READ, 2, readParam);
    // Failed to send
    if (send != 8)
        return -1;
    // Read
    uint8_t result[readLength + 1];
    int rd = recieveMessage(servoId, readLength + 1, result);
    if (rd < 0)
        return rd;

    for (int i = 0; i < readLength; i++)
        outputBuffer[i] = result[i + 1];
    return 0;
}

int STSServoDriver::recieveMessage(uint8_t const& servoId,
                                   uint8_t const& readLength,
                                   uint8_t *outputBuffer)
{
    dirPin_ = 0;
    uint8_t result[readLength + 5];
    size_t rd = port_->read(result, readLength + 5);
    if (rd != readLength + 5)
        return -1;
    // Check message integrity
    if (result[0] != 0xFF || result[1] != 0xFF || result[2] != servoId || result[3] != readLength + 1)
        return -2;
    uint8_t checksum = 0;
    for (int i = 2; i < readLength + 4; i++)
        checksum += result[i];
    checksum = ~checksum;
    if (result[readLength + 4] != checksum)
        return -3;

    // Copy result to output buffer
    for (int i = 0; i < readLength; i++)
        outputBuffer[i] = result[i + 4];
    return 0;
}


void STSServoDriver::flush()
{
	while(port_->readable())
    {
        char ch;
        port_->read(&ch, 1);
    }
}


bool SCSServoDriver::writeTwouint8_tsRegister(uint8_t const& servoId,
                                           uint8_t const& registerId,
                                           int16_t const& value,
                                           bool const& asynchronous)
{
    uint8_t params[2] = {static_cast<unsigned char>((value >> 8) & 0xFF),
                               static_cast<unsigned char>(value & 0xFF)};
    return writeRegisters(servoId, registerId, 2, params, asynchronous);
}


int16_t SCSServoDriver::readTwouint8_tsRegister(uint8_t const& servoId, uint8_t const& registerId)
{
    uint8_t result[2] = {0, 0};
    int rc = readRegisters(servoId, registerId, 2, result);
    if (rc < 0)
        return 0;
    return result[1] + (result[0] << 8);
}

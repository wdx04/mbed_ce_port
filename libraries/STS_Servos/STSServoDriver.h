/// \file STSServoServoDriver.h
/// \brief A simple driver for the STS-serie TTL servos made by Feetech
///
/// \details This code is meant to work as a minimal example for communicating with
///          the STS servos, in particular the low-cost STS-3215 servo.
///          These servos use a communication protocol identical to
///          the Dynamixel serie, but with a different register mapping
///          (due in part to different functionalities like step mode, multiturn...)
#ifndef STSSERVO_DRIVER_H
    #define STSSERVO_DRIVER_H

    #include "mbed.h"

    namespace STS
    {
        enum Registers {
            FIRMWARE_MAJOR          = 0x00,
            FIRMWARE_MINOR          = 0x01,
            SERVO_MAJOR             = 0x03,
            SERVO_MINOR             = 0x04,
            ID                      = 0x05,
            BAUDRATE                = 0x06,
            RESPONSE_DELAY          = 0x07,
            RESPONSE_STATUS_LEVEL   = 0x08,
            MINIMUM_ANGLE           = 0x09,
            MAXIMUM_ANGLE           = 0x0B,
            MAXIMUM_TEMPERATURE     = 0x0D,
            MAXIMUM_VOLTAGE         = 0x0E,
            MINIMUM_VOLTAGE         = 0x0F,
            MAXIMUM_TORQUE          = 0x10,
            PHASE                   = 0x12,
            UNLOADING_CONDITION     = 0x13,
            LED_ALARM_CONDITION     = 0x14,
            POS_PROPORTIONAL_GAIN   = 0x15,
            POS_DERIVATIVE_GAIN     = 0x16,
            POS_INTEGRAL_GAIN       = 0x17,
            MINIMUM_STARTUP_FORCE   = 0x18,
            CK_INSENSITIVE_AREA     = 0x1A,
            CCK_INSENSITIVE_AREA    = 0x1B,
            CURRENT_PROTECTION_TH   = 0x1C,
            ANGULAR_RESOLUTION      = 0x1E,
            POSITION_CORRECTION     = 0x1F,
            OPERATION_MODE          = 0x21,
            TORQUE_PROTECTION_TH    = 0x22,
            TORQUE_PROTECTION_TIME  = 0x23,
            OVERLOAD_TORQUE         = 0x24,
            SPEED_PROPORTIONAL_GAIN = 0x25,
            OVERCURRENT_TIME        = 0x26,
            SPEED_INTEGRAL_GAIN     = 0x27,
            TORQUE_SWITCH           = 0x28,
            TARGET_ACCELERATION     = 0x29,
            TARGET_POSITION         = 0x2A,
            RUNNING_TIME            = 0x2C,
            RUNNING_SPEED           = 0x2E,
            TORQUE_LIMIT            = 0x30,
            WRITE_LOCK              = 0x37,
            CURRENT_POSITION        = 0x38,
            CURRENT_SPEED           = 0x3A,
            CURRENT_DRIVE_VOLTAGE   = 0x3C,
            CURRENT_VOLTAGE         = 0x3E,
            CURRENT_TEMPERATURE     = 0x3F,
            ASYNCHRONOUS_WRITE_ST   = 0x40,
            STATUS                  = 0x41,
            MOVING_STATUS           = 0x42,
            CURRENT_CURRENT         = 0x45,
            PWM_MAX_STEP            = 0x4E,
            MOVING_CHECK_SPEED      = 0x4F,
            D_CONTROL_TIME          = 0x50,
            MIN_SPEED_LIMIT         = 0x51,
            MAX_SPEED_LIMIT         = 0x52,
            ACCELERATION            = 0x53
        };

        enum Instruction {
            PING      = 0x01,
            READ      = 0x02,
            WRITE     = 0x03,
            REGWRITE  = 0x04,
            ACTION    = 0x05,
            SYNCWRITE = 0x83,
            RESET     = 0x06,
        };

    }

    /// \brief Driver for STS servos, using UART
    class STSServoDriver
    {
        public:
            /// \brief Contstructor.
            STSServoDriver();

            virtual ~STSServoDriver();

            /// \brief Initialize the servo driver.
            ///
            /// \param dirPin Pin used for setting communication direction
            /// \param serialPort Serial port, default is Serial
            /// \param baudRate Baud rate, default 1Mbps
            /// \returns  True on success (at least one servo responds to ping)
            bool init(PinName dirPin, BufferedSerial *serialPort, long const& baudRate = 1000000, chrono::milliseconds timeOut = 25ms);

            /// \brief Ping servo
            /// \param[in] servoId ID of the servo
            /// \return True if servo responded to ping
            bool ping(uint8_t const& servoId);

            /// \brief Change the ID of a servo.
            /// \note If the desired ID is already taken, this function does nothing and returns false.
            /// \param[in] oldServoId old servo ID
            /// \param[in] newServoId new servo ID
            /// \return True if servo could successfully change ID
            bool setId(uint8_t const& oldServoId, uint8_t const& newServoId);

            /// \brief Get current servo position.
            /// \note This function assumes that the amplification factor ANGULAR_RESOLUTION is set to 1.
            /// \param[in] servoId ID of the servo
            /// \return Position, in counts. 0 on failure.
            int getCurrentPosition(uint8_t const& servoId);

            /// \brief Get current servo speed.
            /// \note This function assumes that the amplification factor ANGULAR_RESOLUTION is set to 1.
            /// \param[in] servoId ID of the servo
            /// \return Speed, in counts/s. 0 on failure.
            int getCurrentSpeed(uint8_t const& servoId);

            /// \brief Get current servo temperature.
            /// \param[in] servoId ID of the servo
            /// \return Temperature, in degC. 0 on failure.
            int getCurrentTemperature(uint8_t const& servoId);

            /// \brief Get current servo current.
            /// \param[in] servoId ID of the servo
            /// \return Current, in A.
            int getCurrentCurrent(uint8_t const& servoId);

            /// \brief Check if the servo is moving
            /// \param[in] servoId ID of the servo
            /// \return True if moving, false otherwise.
            bool isMoving(uint8_t const& servoId);

            /// \brief Set target servo position.
            /// \note This function assumes that the amplification factor ANGULAR_RESOLUTION is set to 1.
            /// \param[in] servoId ID of the servo
            /// \param[in] position Target position, in counts.
            /// \param[in] asynchronous If set, write is asynchronous (ACTION must be send to activate)
            /// \return True on success, false otherwise.
            bool setTargetPosition(uint8_t const& servoId, int const& position, bool const& asynchronous = false);

            /// \brief Set target servo velocity.
            /// \note This function assumes that the amplification factor ANGULAR_RESOLUTION is set to 1.
            /// \param[in] servoId ID of the servo
            /// \param[in] velocity Target velocity, in counts/s.
            /// \param[in] asynchronous If set, write is asynchronous (ACTION must be send to activate)
            /// \return True on success, false otherwise.
            bool setTargetVelocity(uint8_t const& servoId, int const& velocity, bool const& asynchronous = false);

            /// \brief Trigger the action previously stored by an asynchronous write on all servos.
            /// \return True on success
            bool trigerAction();

            /// \brief Write to a single uint8_t register.
            /// \param[in] servoId ID of the servo
            /// \param[in] registerId Register id.
            /// \param[in] value Register value.
            /// \param[in] asynchronous If set, write is asynchronous (ACTION must be send to activate)
            /// \return True if write was successful
            bool writeRegister(uint8_t const& servoId,
                               uint8_t const& registerId,
                               uint8_t const& value,
                               bool const& asynchronous = false);

            /// \brief Read a single register
            /// \param[in] servoId ID of the servo
            /// \param[in] registerId Register id.
            /// \return Register value, 0 on failure.
            uint8_t readRegister(uint8_t const& servoId, uint8_t const& registerId);

            /// \brief Write a two-uint8_ts register.
            /// \param[in] servoId ID of the servo
            /// \param[in] registerId Register id (LSB).
            /// \param[in] value Register value.
            /// \param[in] asynchronous If set, write is asynchronous (ACTION must be send to activate)
            /// \return True if write was successful
            virtual bool writeTwouint8_tsRegister(uint8_t const& servoId,
                                       uint8_t const& registerId,
                                       int16_t const& value,
                                       bool const& asynchronous = false);

            /// \brief Read two uint8_ts, interpret result as <LSB> <MSB>
            /// \param[in] servoId ID of the servo
            /// \param[in] registerId LSB register id.
            /// \return Register value, 0 on failure.
            virtual int16_t readTwouint8_tsRegister(uint8_t const& servoId, uint8_t const& registerId);

        protected:
            /// \brief Clear internal device error.
            // void clearError();

            /// \brief Send a message to the servos.
            /// \param[in] servoId ID of the servo
            /// \param[in] commandID Command id
            /// \param[in] paramLength length of the parameters
            /// \param[in] parameters parameters
            /// \return Result of write.
            int sendMessage(uint8_t const& servoId,
                            uint8_t const& commandID,
                            uint8_t const& paramLength,
                            uint8_t *parameters);

            /// \brief Recieve a message from a given servo.
            /// \param[in] servoId ID of the servo
            /// \param[in] readLength Message length
            /// \param[in] paramLength length of the parameters
            /// \param[in] outputBuffer Buffer where the data is placed.
            /// \return 0 on success
            ///         -1 if read failed due to timeout
            ///         -2 if invalid message (no 0XFF, wrong servo id)
            ///         -3 if invalid checksum
            int recieveMessage(uint8_t const& servoId,
                               uint8_t const& readLength,
                               uint8_t *outputBuffer);

            /// \brief Write to a sequence of consecutive registers
            /// \param[in] servoId ID of the servo
            /// \param[in] startRegister First register
            /// \param[in] writeLength Number of registers to write
            /// \param[in] parameters Value of the registers
            /// \param[in] asynchronous If set, write is asynchronous (ACTION must be send to activate)
            /// \return True if write was successful
            bool writeRegisters(uint8_t const& servoId,
                                uint8_t const& startRegister,
                                uint8_t const& writeLength,
                                uint8_t const*parameters,
                                bool const& asynchronous = false);

            /// \brief Read a sequence of consecutive registers.
            /// \param[in] servoId ID of the servo
            /// \param[in] startRegister First register
            /// \param[in] readLength Number of registers to write
            /// \param[out] outputBuffer Buffer where to read the data (must have been allocated by the user)
            /// \return 0 on success, -1 if write failed, -2 if read failed, -3 if checksum verification failed
            int readRegisters(uint8_t const& servoId,
                              uint8_t const& startRegister,
                              uint8_t const& readLength,
                              uint8_t *outputBuffer);

            /// \brief Discard any unread data in the read buffer.
            void flush();  

            BufferedSerial *port_ { nullptr };
            DigitalOut dirPin_ { NC };     ///< Direction pin
            chrono::milliseconds timeOut_;
    };

    /// \brief Driver for SCS servos, using UART
    /// \note Byte order for CSC is different than STS
    class SCSServoDriver
        : public STSServoDriver
    {
    public:    
            /// \brief Write a two-uint8_ts register.
            /// \param[in] servoId ID of the servo
            /// \param[in] registerId Register id (LSB).
            /// \param[in] value Register value.
            /// \param[in] asynchronous If set, write is asynchronous (ACTION must be send to activate)
            /// \return True if write was successful
            bool writeTwouint8_tsRegister(uint8_t const& servoId,
                                       uint8_t const& registerId,
                                       int16_t const& value,
                                       bool const& asynchronous = false) override;

            /// \brief Read two uint8_ts, interpret result as <LSB> <MSB>
            /// \param[in] servoId ID of the servo
            /// \param[in] registerId LSB register id.
            /// \return Register value, 0 on failure.
            int16_t readTwouint8_tsRegister(uint8_t const& servoId, uint8_t const& registerId) override;
    };
#endif

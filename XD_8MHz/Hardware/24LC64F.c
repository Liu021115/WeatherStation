#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "24LC64F.h"
#include "protocol_def.h"

#define M24LC64F_ADDRESS		0xA0		//24LC64F的I2C从机地址


/**
  * 函    数：24LC64F初始化
  * 参    数：无
  * 返 回 值：无
  * 注意事项：此函数需要用户实现内容，实现SCL和SDA引脚的初始化
  */
void M24LC64F_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	//开启GPIOC的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);					//将PC7和PC8引脚初始化为开漏输出
	
	/*设置默认电平*/
	GPIO_SetBits(GPIOC, GPIO_Pin_7 | GPIO_Pin_8);			//设置PC7和PC8引脚初始化后默认为高电平（释放总线状态）
}

/*********************************************
 * 函数名：I2Cdelay
 * 描  述：I2C延时函数
 * 输  入：无
 * 输  出：无
 ********************************************/
static void I2Cdelay(void)
{	
		Delay_us(20); //这里可以优化速度，通常延时3~10us，可以用示波器看波形来调试
}

/**
  * 函    数：I2C读SDA引脚电平
  * 参    数：无
  * 返 回 值：协议层需要得到的当前SDA的电平，范围0~1
  * 注意事项：此函数需要用户实现内容，当前SDA为低电平时，返回0，当前SDA为高电平时，返回1
  */
uint8_t MyI2C_R_SDA(void)
{
	uint8_t BitValue;
	BitValue = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8);		//读取SDA电平
	Delay_us(10);												//延时10us，防止时序频率超过要求
	return BitValue;											//返回SDA电平
}

/**
  * 函    数：I2C写SCL引脚电平
  * 参    数：BitValue 协议层传入的当前需要写入SCL的电平，范围0~1
  * 返 回 值：无
  * 注意事项：此函数需要用户实现内容，当BitValue为0时，需要置SCL为低电平，当BitValue为1时，需要置SCL为高电平
  */
void MyI2C_W_SCL(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOC, GPIO_Pin_7, (BitAction)BitValue);		//根据BitValue，设置SCL引脚的电平
	I2Cdelay();												//延时10us，防止时序频率超过要求
}

/**
  * 函    数：I2C写SDA引脚电平
  * 参    数：BitValue 协议层传入的当前需要写入SDA的电平，范围0~1
  * 返 回 值：无
  * 注意事项：此函数需要用户实现内容，当BitValue为0时，需要置SDA为低电平，当BitValue为1时，需要置SDA为高电平
  */
void MyI2C_W_SDA(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOC, GPIO_Pin_8, (BitAction)BitValue);		//根据BitValue，设置SDA引脚的电平，BitValue要实现非0即1的特性
	I2Cdelay();												//延时10us，防止时序频率超过要求
}

/**
  * 函    数：I2C起始
  * 参    数：无
  * 返 回 值：0操作成功 1操作失败
  */
static u8 MyI2C_Start(void)
{
	MyI2C_W_SDA(1);							//释放SDA，确保SDA为高电平
	MyI2C_W_SCL(1);							//释放SCL，确保SCL为高电平
	if(MyI2C_R_SDA()==0) return 1;				//SDA线为低电平则总线忙,退出
	MyI2C_W_SDA(0);							//在SCL高电平期间，拉低SDA，产生起始信号
	MyI2C_W_SCL(0);							//起始后把SCL也拉低，即为了占用总线，也为了方便总线时序的拼接
	return 0;
}

/**
  * 函    数：I2C终止
  * 参    数：无
  * 返 回 值：无
  */
void MyI2C_Stop(void)
{
	MyI2C_W_SCL(0);
	MyI2C_W_SDA(0);							//拉低SDA，确保SDA为低电平
	MyI2C_W_SCL(1);							//释放SCL，使SCL呈现高电平
	MyI2C_W_SDA(1);							//在SCL高电平期间，释放SDA，产生终止信号
}

/**
  * 函    数：I2C发送应答位
  * 参    数：Byte 要发送的应答位，范围：0~1，0表示应答，1表示非应答
  * 返 回 值：无
  */
void MyI2C_SendAck(uint8_t AckBit)
{
	MyI2C_W_SCL(0);
	MyI2C_W_SDA(AckBit);					//主机把应答位数据放到SDA线
	MyI2C_W_SCL(1);							//释放SCL，从机在SCL高电平期间，读取应答位
	MyI2C_W_SCL(0);							//拉低SCL，开始下一个时序模块
}

/**
  * 函    数：I2C接收应答位
  * 参    数：无
  * 返 回 值：接收到的应答位，范围：0~1，0表示应答，1表示非应答
  */
uint8_t MyI2C_ReceiveAck(void)
{
	uint8_t AckBit;							//定义应答位变量
	MyI2C_W_SCL(0);
	MyI2C_W_SDA(1);							//接收前，主机先确保释放SDA，避免干扰从机的数据发送
	MyI2C_W_SCL(1);							//释放SCL，主机机在SCL高电平期间读取SDA
	Delay_us(10);
	AckBit = MyI2C_R_SDA();					//将应答位存储到变量里
	MyI2C_W_SCL(0);							//拉低SCL，开始下一个时序模块
	return AckBit;							//返回定义应答位变量
}


/**
  * 函    数：I2C发送一个字节
  * 参    数：Byte 要发送的一个字节数据，范围：0x00~0xFF
  * 返 回 值：无
  */
void MyI2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i ++)				//循环8次，主机依次发送数据的每一位
	{
		MyI2C_W_SCL(0);
		/*两个!可以对数据进行两次逻辑取反，作用是把非0值统一转换为1，即：!!(0) = 0，!!(非0) = 1*/
		MyI2C_W_SDA(!!(Byte & (0x80 >> i)));//使用掩码的方式取出Byte的指定一位数据并写入到SDA线
		Delay_us(10);
		MyI2C_W_SCL(1);						//释放SCL，从机在SCL高电平期间读取SDA
		Delay_us(10);
	}
	MyI2C_W_SCL(0);						//拉低SCL
}

/**
  * 函    数：I2C接收一个字节
  * 参    数：无
  * 返 回 值：接收到的一个字节数据，范围：0x00~0xFF
  */
uint8_t MyI2C_ReceiveByte(void)
{
	uint8_t i, Byte = 0x00;					//定义接收的数据，并赋初值0x00，此处必须赋初值0x00，后面会用到
	MyI2C_W_SDA(1);							//接收前，主机先确保释放SDA，避免干扰从机的数据发送
	for (i = 0; i < 8; i ++)				//循环8次，主机依次接收数据的每一位
	{
		MyI2C_W_SCL(1);						//释放SCL，主机机在SCL高电平期间读取SDA
		if (MyI2C_R_SDA()){Byte |= (0x80 >> i);}	//读取SDA数据，并存储到Byte变量
													//当SDA为1时，置变量指定位为1，当SDA为0时，不做处理，指定位为默认的初值0
		MyI2C_W_SCL(0);						//拉低SCL，从机在SCL低电平期间写入SDA
	}
	return Byte;							//返回接收到的一个字节数据
}




/**
  * 函    数：24LC64F写寄存器  写入最多32字节 
  * 参    数：RegAddress 寄存器地址，分高八位和低八位
  * 参    数：Data 要写入寄存器的数据，范围：0x00~0xFF
  * 返 回 值：0正常  -1错误码
  */
uint8_t M24LC64F_WriteReg(uint16_t RegAddress, uint8_t *Data,uint8_t SendLen)
{
	
	
	if(MyI2C_Start()!=0) return 1;														//I2C起始
	MyI2C_SendByte(M24LC64F_ADDRESS);													//发送从机地址，读写位为0，表示即将写入
	if(MyI2C_ReceiveAck()==1){MyI2C_Stop(); return 1;}					//接收应答
	MyI2C_SendByte(RegAddress>>8);														//发送寄存器地址
	MyI2C_ReceiveAck();																				//接收应答
	MyI2C_SendByte(RegAddress & 0x00FF);			//发送寄存器地址
	MyI2C_ReceiveAck();					//接收应答
	for(uint8_t i=0;i<SendLen;i++)
	{
		MyI2C_SendByte(Data[i]);				//发送要写入寄存器的数据
		MyI2C_ReceiveAck();					//接收应答
	}
	MyI2C_Stop();						//I2C终止
	Delay_ms(10);            //等待EEPROM内部写入完成
	return 0;
}

/**
  * 函    数：24LC64F读寄存器
  * 参    数：RegAddress 寄存器地址，范围：参考24LC64F手册的寄存器描述
  * 返 回 值：读取寄存器的数据，范围：0x00~0xFF
  */
uint8_t M24LC64F_ReadReg(uint16_t RegAddress,uint8_t *Data,uint8_t ReceiveLen)
{	
	MyI2C_Start();						//I2C起始
	MyI2C_SendByte(M24LC64F_ADDRESS);	//发送从机地址，读写位为0，表示即将读取
	MyI2C_ReceiveAck();					//接收应答
	MyI2C_SendByte(RegAddress>>8);			//发送寄存器地址
	MyI2C_ReceiveAck();					//接收应答
	MyI2C_SendByte(RegAddress & 0xFF);			//发送寄存器地址
	if(MyI2C_ReceiveAck()==1){MyI2C_Stop();return 1;}					//接收应答	
	
	
	MyI2C_Start();						//I2C重复起始
	MyI2C_SendByte(M24LC64F_ADDRESS | 0x01);	//发送从机地址，读写位为1，表示即将读取
	if(MyI2C_ReceiveAck()==1){MyI2C_Stop();return 1;}					//接收应答
	
	for(uint8_t i=0;i<ReceiveLen;i++){
		Data[i] = MyI2C_ReceiveByte();			//接收指定寄存器的数据
		MyI2C_SendAck(i==(ReceiveLen-1) ? 1:0);
		Delay_us(10);
	}
	MyI2C_Stop();						//I2C终止
	return 0;
}

/**
  * 函    数：24LC64F读设备信息
  * 参    数：RegAddress 寄存器地址，范围：参考24LC64F手册的寄存器描述
  * 返 回 值：读取寄存器的数据
  */
void ReadInfor24LC64F(uint8_t *ID,uint8_t *PassWord,uint8_t *IP,uint8_t *Port)
{
	M24LC64F_ReadReg(EEPROM_ID,ID,32);									//读取EEPROM存放的ID，读取长度32位
	Delay_ms(10);
	M24LC64F_ReadReg(EEPROM_PASSWORD,PassWord,32);			//读取EEPROM存放的密码，读取长度32位
	Delay_ms(10);
	M24LC64F_ReadReg(EEPROM_IP,IP,32);									//读取EEPROM存放的IP，读取长度32位
	Delay_ms(10);
	M24LC64F_ReadReg(EEPROM_PORT,Port,32);							//读取EEPROM存放的端口号，读取长度32位
	Delay_ms(10);
	M24LC64F_ReadReg(EEPROM_LAT,(uint8_t *)positioning.Lat,16);      //读取EEPROM存放的纬度信息，读取长度16位
	Delay_ms(10);
	M24LC64F_ReadReg(EEPROM_LON,(uint8_t *)positioning.Lon,16);       //读取EEPROM存放的经度信息，读取长度16位
	Delay_ms(10);
}


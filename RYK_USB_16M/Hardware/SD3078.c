#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "SD3078.h"


#define SD3078_ADDRESS		0x64		//SD3078的I2C从机地址
// 寄存器地址定义
#define DS3231_SECOND       	0x00  //秒
#define DS3231_MINUTE       	0x01  //分
#define DS3231_HOUR        		0x02	//时
#define DS3231_WEEK         	0x03	//星期
#define DS3231_DAY          	0x04	//日
#define DS3231_MONTH        	0x05 	//月
#define DS3231_YEAR         	0x06  //年
#define CHARGE_REG 0x18 // 充电控制寄存器

// I2C引脚定义
#define SD3078_I2C_SCL_PORT GPIOB
#define SD3078_I2C_SCL_PIN  GPIO_Pin_8
#define SD3078_I2C_SDA_PORT GPIOB
#define SD3078_I2C_SDA_PIN  GPIO_Pin_9

// BCD转十进制
uint8_t BCD2DEC(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}
// 十进制转BCD
uint8_t DEC2BCD(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}




/*引脚配置层*/
/**
  * 函    数：I2C写SCL引脚电平
  * 参    数：BitValue 协议层传入的当前需要写入SCL的电平，范围0~1
  * 返 回 值：无
  * 注意事项：此函数需要用户实现内容，当BitValue为0时，需要置SCL为低电平，当BitValue为1时，需要置SCL为高电平
  */
void SD3078_I2C_W_SCL(uint8_t BitValue)
{
	GPIO_WriteBit(SD3078_I2C_SCL_PORT, SD3078_I2C_SCL_PIN, (BitAction)BitValue);		//根据BitValue，设置SCL引脚的电平
	Delay_us(10);												//延时10us，防止时序频率超过要求
}

/**
  * 函    数：I2C写SDA引脚电平
  * 参    数：BitValue 协议层传入的当前需要写入SDA的电平，范围0~1
  * 返 回 值：无
  * 注意事项：此函数需要用户实现内容，当BitValue为0时，需要置SDA为低电平，当BitValue为1时，需要置SDA为高电平
  */
void SD3078_I2C_W_SDA(uint8_t BitValue)
{
	GPIO_WriteBit(SD3078_I2C_SDA_PORT, SD3078_I2C_SDA_PIN, (BitAction)BitValue);		//根据BitValue，设置SDA引脚的电平，BitValue要实现非0即1的特性
	Delay_us(10);												//延时10us，防止时序频率超过要求
}

/**
  * 函    数：I2C读SDA引脚电平
  * 参    数：无
  * 返 回 值：协议层需要得到的当前SDA的电平，范围0~1
  * 注意事项：此函数需要用户实现内容，当前SDA为低电平时，返回0，当前SDA为高电平时，返回1
  */
uint8_t SD3078_I2C_R_SDA(void)
{
	static uint8_t BitValue;
	BitValue = GPIO_ReadInputDataBit(SD3078_I2C_SDA_PORT, SD3078_I2C_SDA_PIN);		//读取SDA电平
	Delay_us(10);												//延时10us，防止时序频率超过要求
	return BitValue;											//返回SDA电平
}

/**
  * 函    数：I2C初始化
  * 参    数：无
  * 返 回 值：无
  * 注意事项：此函数需要用户实现内容，实现SCL和SDA引脚的初始化
  */
void SD3078_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//开启GPIOB的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Pin = SD3078_I2C_SCL_PIN | SD3078_I2C_SDA_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);					//将PB10和PB11引脚初始化为开漏输出
	
	/*设置默认电平*/
	GPIO_SetBits(GPIOB, SD3078_I2C_SCL_PIN | SD3078_I2C_SDA_PIN);			//设置PB10和PB11引脚初始化后默认为高电平（释放总线状态）
}

/*协议层*/

/**
  * 函    数：I2C起始
  * 参    数：无
  * 返 回 值：无
  */
void Start(void)
{
	SD3078_I2C_W_SDA(1);							//释放SDA，确保SDA为高电平
	SD3078_I2C_W_SCL(1);							//释放SCL，确保SCL为高电平
	SD3078_I2C_W_SDA(0);							//在SCL高电平期间，拉低SDA，产生起始信号
	SD3078_I2C_W_SCL(0);							//起始后把SCL也拉低，即为了占用总线，也为了方便总线时序的拼接
}

/**
  * 函    数：I2C终止
  * 参    数：无
  * 返 回 值：无
  */
void Stop(void)
{
	SD3078_I2C_W_SDA(0);							//拉低SDA，确保SDA为低电平
	SD3078_I2C_W_SCL(1);							//释放SCL，使SCL呈现高电平
	SD3078_I2C_W_SDA(1);							//在SCL高电平期间，释放SDA，产生终止信号
}

/**
  * 函    数：I2C发送一个字节
  * 参    数：Byte 要发送的一个字节数据，范围：0x00~0xFF
  * 返 回 值：无
  */
void SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i ++)				//循环8次，主机依次发送数据的每一位
	{
		/*两个!可以对数据进行两次逻辑取反，作用是把非0值统一转换为1，即：!!(0) = 0，!!(非0) = 1*/
		SD3078_I2C_W_SDA(!!(Byte & (0x80 >> i)));//使用掩码的方式取出Byte的指定一位数据并写入到SDA线
		SD3078_I2C_W_SCL(1);						//释放SCL，从机在SCL高电平期间读取SDA
		SD3078_I2C_W_SCL(0);						//拉低SCL，主机开始发送下一位数据
	}
}

/**
  * 函    数：I2C接收一个字节
  * 参    数：无
  * 返 回 值：接收到的一个字节数据，范围：0x00~0xFF
  */
uint8_t ReceiveByte(void)
{
	uint8_t i, Byte = 0x00;					//定义接收的数据，并赋初值0x00，此处必须赋初值0x00，后面会用到
	SD3078_I2C_W_SDA(1);							//接收前，主机先确保释放SDA，避免干扰从机的数据发送
	for (i = 0; i < 8; i ++)				//循环8次，主机依次接收数据的每一位
	{
		SD3078_I2C_W_SCL(1);						//释放SCL，主机机在SCL高电平期间读取SDA
		if (SD3078_I2C_R_SDA()){Byte |= (0x80 >> i);}	//读取SDA数据，并存储到Byte变量
													//当SDA为1时，置变量指定位为1，当SDA为0时，不做处理，指定位为默认的初值0
		SD3078_I2C_W_SCL(0);						//拉低SCL，从机在SCL低电平期间写入SDA
	}
	return Byte;							//返回接收到的一个字节数据
}

/**
  * 函    数：I2C发送应答位
  * 参    数：Byte 要发送的应答位，范围：0~1，0表示应答，1表示非应答
  * 返 回 值：无
  */
void SendAck(uint8_t AckBit)
{
	SD3078_I2C_W_SDA(AckBit);					//主机把应答位数据放到SDA线
	SD3078_I2C_W_SCL(1);							//释放SCL，从机在SCL高电平期间，读取应答位
	SD3078_I2C_W_SCL(0);							//拉低SCL，开始下一个时序模块
}

/**
  * 函    数：I2C接收应答位
  * 参    数：无
  * 返 回 值：接收到的应答位，范围：0~1，0表示应答，1表示非应答
  */
uint8_t ReceiveAck(void)
{
	static uint8_t AckBit;							//定义应答位变量
	SD3078_I2C_W_SDA(1);							//接收前，主机先确保释放SDA，避免干扰从机的数据发送
	SD3078_I2C_W_SCL(1);							//释放SCL，主机机在SCL高电平期间读取SDA
	AckBit = SD3078_I2C_R_SDA();					//将应答位存储到变量里
	SD3078_I2C_W_SCL(0);							//拉低SCL，开始下一个时序模块
	return AckBit;							//返回定义应答位变量
}
/**
  * 函    数：SD3078写寄存器
  * 参    数：RegAddress 寄存器地址，范围：参考SD3078手册的寄存器描述
  * 参    数：Data 要写入寄存器的数据，范围：0x00~0xFF
  * 返 回 值：无
  */
void SD3078_WriteReg(uint8_t Data,uint8_t RegAddress)
{
	Start();						//I2C起始
	SendByte(SD3078_ADDRESS);	//发送从机地址，读写位为0，表示即将写入
	ReceiveAck();					//接收应答
	SendByte(RegAddress);			//发送寄存器地址
	ReceiveAck();					//接收应答
	SendByte(Data);				//发送要写入寄存器的数据
	ReceiveAck();					//接收应答
	Stop();						//I2C终止
}
/**
  * 函    数：SD3078读寄存器
  * 参    数：RegAddress 寄存器地址，范围：参考24LC64F手册的寄存器描述
  * 返 回 值：读取寄存器的数据，范围：0x00~0xFF
  */
uint8_t SD3078_ReadReg(uint8_t RegAddress)
{
	static uint8_t Data;
	
	Start();						//I2C起始
	SendByte(SD3078_ADDRESS);	//发送从机地址，读写位为0，表示即将写入
	ReceiveAck();					//接收应答
	SendByte(RegAddress);			//发送寄存器地址
	ReceiveAck();					//接收应答
	
	Start();						//I2C重复起始
	SendByte(SD3078_ADDRESS | 0x01);	//发送从机地址，读写位为1，表示即将读取
	ReceiveAck();					//接收应答
	Data = ReceiveByte();			//接收指定寄存器的数据
	SendAck(1);					//发送应答，给从机非应答，终止从机的数据输出
	Stop();						//I2C终止
	
	return Data;
}


void ModifyTime(uint8_t year,uint8_t mon,uint8_t day,uint8_t hou,uint8_t min,uint8_t sec)
{
    uint8_t temp=0;
   
    temp=DEC2BCD(year);
    SD3078_WriteReg(DS3231_YEAR,temp);   //修改年
   
    temp=DEC2BCD(mon);
    SD3078_WriteReg(DS3231_MONTH,temp);  //修改月
   
    temp=DEC2BCD(day);
    SD3078_WriteReg(DS3231_DAY,temp);    //修改日
   
    temp=DEC2BCD(hou);
    SD3078_WriteReg(DS3231_HOUR,temp);   //修改时
   
    temp=DEC2BCD(min);
    SD3078_WriteReg(DS3231_MINUTE,temp); //修改分
   
    temp=DEC2BCD(sec);
    SD3078_WriteReg(DS3231_SECOND,temp); //修改秒
}

/*//读时间
				Time[0] = BCD2DEC(SD3078_ReadReg(DS3231_YEAR));
				Time[1] = BCD2DEC(SD3078_ReadReg(DS3231_MONTH)&0x1f);
				Time[2] = BCD2DEC(SD3078_ReadReg(DS3231_DAY));
				Time[3] = BCD2DEC(SD3078_ReadReg(DS3231_HOUR));
				Time[4] = BCD2DEC(SD3078_ReadReg(DS3231_MINUTE));
				Time[5] = BCD2DEC(SD3078_ReadReg(DS3231_SECOND));
*/
int ReadTime_SD3078(	DateTime *dt)
{
	  uint8_t sec = SD3078_ReadReg(DS3231_SECOND);
    uint8_t min = SD3078_ReadReg(DS3231_MINUTE);
    uint8_t hour = SD3078_ReadReg(DS3231_HOUR);
    uint8_t day = SD3078_ReadReg(DS3231_DAY);
    uint8_t month = SD3078_ReadReg(DS3231_MONTH);
    uint8_t year = SD3078_ReadReg(DS3231_YEAR);
	
		dt->sec = BCD2DEC(sec);
    dt->min = BCD2DEC(min);
    dt->hour = BCD2DEC(hour);  // 假设使用24小时制，若SD3078是12小时制需额外处理
    dt->day = BCD2DEC(day);
    dt->month = BCD2DEC(month & 0x1F);  // 清除月份寄存器的最高位（可能为 Century 标志）
    dt->year = BCD2DEC(year) ; 
	
	// 简单校验时间合理性（可选）
    if (dt->month < 1 || dt->month > 12 || dt->day < 1 || dt->day > 31 || 
        dt->hour > 23 || dt->min > 59 || dt->sec > 59) {
        return 1;  // 时间无效
    }
    return 0;  // 成功
	
}




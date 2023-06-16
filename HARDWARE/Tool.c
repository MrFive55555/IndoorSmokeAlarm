#include "Tool.h"
#include "FreeRTOS.h"
#include "Usart.h"
/**
*工具类:为程序提供各种小功能
*/
//数字转字符串
u8* intNumToStr(u32 num){
	//分配内存，放在堆区
	u8 i,length = 0,midLength,temp;
//	u8 *number = NULL, *str = NULL;
//	number = (u8 *)malloc(sizeof(u8));
	u8 *number = (u8*)pvPortMalloc(1);
	u8 *str = number; //保存number的首地址
	//取出每一个数字 逆序存储
	while(num>0){
		*number++ = num%10+'0';
		num/=10;
		length++;
	}
	*number = '\0';
	//字符反转
	midLength = length/2;
	for(i=0;i<midLength;i++){
		//不能使用number[i] 应该使用str[i] 因为此时number的首地址已经发生改变
		temp = str[i];
		str[i] = str[length-1];
		str[length-1] = temp;
		length--;
	}
	return str;
}
/**
*！！！大问题：好像会卡在floatNumToStr无限循环
*原因：
1.可能是ADC中断和写入冲突 导致中断一直执行 主函数无法执行 x
2.floatNumToStr存在问题 导致整个程序卡死 √
测试2存在问题 无论是嵌套函数调用 还是单独函数调用
但是2中并没有无限循环 intNumToStr没有问题
解决方案:
1.默认堆空间过小 malloc分配的内存放在堆中导致堆溢出且没有及时释放
所以扩大堆空间将0x200修改成0xC00
*/
u8 *floatNumToStr(float num,u8 decimalPlace){
	u8 length = 0,midLength = 0;
	u8 i, temp;
	//u8 *number = NULL, *str = NULL;
	//动态分配内存 一定要释放
	//number = (u8 *)malloc(sizeof(u8));
	u8 *number = (u8*)pvPortMalloc(1); //使用freertos内存管理分配内存
	u8 *str = number; //保存字符串number首地址
	u32 newNum;
	//扩大小数为整数 最大为2^32-1
	for (i = 0; i < decimalPlace; i++) num *= 10;
	newNum = (u32) num;
	if(newNum%10!=0) newNum+=1; //因为精度问题,newNum为非10整数，所以需要四舍五入
	while (newNum > 0) {
		*number++ = newNum % 10 + '0';
		newNum /= 10;
		length++;
		if (length == decimalPlace) { //小数点赋值位置
			*number++ = '.';
			length++;
		}
	}
	//小数点不建议充当判断条件
	if(newNum<100||newNum==0){
		//添加小数点前显示0(仅限两位小数)
		if (length == decimalPlace + 1) { //1位0
			*number++ = '0';
			length++;
		} else if (length == 1) { //2位0
			for (i = 0; i < 2; i++) {
				*(char *)number++ = '0';
				length++;
				if (length == decimalPlace) {
					*number++ = '.';
					length++;
				}
			}
		} else if (length == 0) { //全为0
			for (i = 0; i < 3; i++) {
				*number++ = '0';
				length++;
				if (length == decimalPlace) {
					*number++ = '.';
					length++;
				}
			}
		}
	}
	*number = '\0';
	midLength = length / 2;
	for (i = 0; i < midLength; i++) {
		temp = str[i];
		str[i] = str[length - 1];
		str[length - 1] = temp;
		length--;
	}
	return str; //返回字符串首地址
}
/**
问题：
float转int存在精度问题，导致无法正确获得数值
解决方案:四舍五入
*/

//字符串转浮点数
float strToFloat(u8 *str,u8 numPlace){ //字符串，位数
	float number = 0.0f;
	u32 i = 1;
	u32 powNum = pow10(numPlace);
	//printf("start convert str to float is %d\r\n",powNum);
	while(*str!='\0'){
		//printf("*str is %c\r\n",*str);
		if(*str!='.'){
			number += (float)(*str - '0')*powNum/(float)i; 
			i*=10;
			//printf("number is %f\r\n",number);
		}
		str++;
	}
	return number/=powNum;
}

//获取1*10的次方
u32 pow10(u8 numPlace){
	u32 number = 1;
	while(numPlace){
		number*=10;
		numPlace--;
	}
	return number;
}

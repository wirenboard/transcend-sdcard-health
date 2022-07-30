/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 *
 * Modified to add field firmware update support,
 * those modifications are Copyright (c) 2016 SanDisk Corp.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>
#include <linux/fs.h> /* for BLKGETSIZE */
#include "mmc.h"
#include "mmc_cmds.h"

enum rpmb_op_type {
	MMC_RPMB_WRITE_KEY = 0x01,
	MMC_RPMB_READ_CNT  = 0x02,
	MMC_RPMB_WRITE     = 0x03,
	MMC_RPMB_READ      = 0x04,

	/* For internal usage only, do not use it directly */
	MMC_RPMB_READ_RESP = 0x05
};

struct rpmb_frame {
	u_int8_t  stuff[196];
	u_int8_t  key_mac[32];
	u_int8_t  data[256];
	u_int8_t  nonce[16];
	u_int32_t write_counter;
	u_int16_t addr;
	u_int16_t block_count;
	u_int16_t result;
	u_int16_t req_resp;
};

static inline void set_single_cmd(struct mmc_ioc_cmd *ioc, __u32 opcode,
				  int write_flag, unsigned int blocks)
{
	ioc->opcode = opcode;
	ioc->write_flag = write_flag;
	ioc->arg = 0x0;
	ioc->blksz = 512;
	ioc->blocks = blocks;
	ioc->flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
}

int do_SMART_buffer_dump(int nargs, char **argv)/* Show SMART 512-byte buffer by using CMD56 */
{
	int argCmd56 = 0x110005F9;
	char data_buff[SD_SMT_BLOCK_SIZE];
	int fd, ret;
	char *device;
	device = argv[nargs-1];
	fd = open(device, O_RDWR);
	if (fd < 0) {
		perror("open file fail");
		exit(1);
	}
	ret = CMD56_data_in(fd, argCmd56 , data_buff);
	if (ret) {
	fprintf(stderr, "CMD56 function fail, %s\n", device);
	exit(1);
	}
	dump_smart_data(data_buff); 	
	return ret;
}

int show_SMART_info(int nargs, char **argv) /* Show SMART info (ex: Speed class/ Erase count / FW version, etc...)  */
{
	int argCmd56 = 0x110005F9;
	char data_buff[SD_SMT_BLOCK_SIZE];
	int fd, ret;
	char *device;
	device = argv[nargs-1];
	fd = open(device, O_RDWR);
	if (fd < 0) {
		perror("open file fail");
		exit(1);
	}
	ret = CMD56_data_in(fd, argCmd56 , data_buff);
	if (ret) {
	fprintf(stderr, "CMD56 function fail, %s\n", device);
	exit(1);
	}
	is_transcend_card(data_buff, TYPE_SMART); /* Only support microSDXC430T and microSDXC450I */
	return ret;
}

int show_Health_info(int nargs, char **argv) /* Show Health */
{
	int argCmd56 = 0x110005F9;
	char data_buff[SD_SMT_BLOCK_SIZE];
	int fd, ret;
	char *device;
	device = argv[nargs-1];
	fd = open(device, O_RDWR);
	if (fd < 0) {
		perror("open file fail");
		exit(1);
	}
	//Get one 512-byte data block by CMD56 function
	ret = CMD56_data_in(fd, argCmd56 , data_buff);
	if (ret) {
	fprintf(stderr, "CMD56 function fail, %s\n", device);
	exit(1);
	}
	is_transcend_card(data_buff, TYPE_HEALTH); //Transcend SD card only
	return ret;
}

int CMD56_data_in(int fd, int argCmd56, char *block_data_buff)
{
	int ret = 0;
	struct mmc_ioc_cmd ioc_data;
	memset(&ioc_data, 0, sizeof(ioc_data));
	memset(block_data_buff, 0, sizeof(__u8) * SD_SMT_BLOCK_SIZE);
	ioc_data.arg = argCmd56;
	ioc_data.write_flag = 0;
	ioc_data.blocks = 1;
	ioc_data.blksz = SD_SMT_BLOCK_SIZE;
	ioc_data.opcode = SD_CMD;
	ioc_data.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	mmc_ioc_cmd_set_data(ioc_data, block_data_buff);
	ret = ioctl(fd, MMC_IOC_CMD, &ioc_data);
	if (ret)
		perror("ioctl fail");
	return ret;
}
void dump_smart_data(char *block_data_buff)
{
	int count=0;
	printf("SMART data block:");
	while( count < SD_SMT_BLOCK_SIZE) {
		if(count % 16 == 0)
		printf("\n%03d: ", count);
		printf("%02x ", block_data_buff[count]);
		count++;
	}
	printf("\n");
	
	return;
}

void is_transcend_card(char *block_data_buff, char function)
{
	if(block_data_buff[0] == 84 && block_data_buff[1] == 114 && block_data_buff[2] == 97 && block_data_buff[3] == 110 && block_data_buff[4] == 115 
		&& block_data_buff[5] == 99 && block_data_buff[6] == 101 && block_data_buff[7] == 110 && block_data_buff[8] == 100)//TRANSCEND
	{
		char controllerChar[9];
		strncpy(controllerChar, block_data_buff + 88, 8);
		if(strstr(controllerChar, "SM2706"))
		{
			if(function == TYPE_SMART)
			{
				parsing_SMART_info(block_data_buff);
			}
			else if(function == TYPE_HEALTH)
			{
				parsing_Health_info(block_data_buff);
			}
		}
		else
		{
			printf("Not Support.\n");
		}
	}
	else
	{
		printf("Not Support.\n");
	}
	return;
}

void parsing_SMART_info(char *block_data_buff) /* parsing SMART 512-byte array */ 
{
	char value[64];
	
	char *card_marker = grabString(block_data_buff, 0, 16);
	sprintf(value, "Card Marker(0x00):\t\t\t%s", card_marker);
	printf("\n%s", value);
	free(card_marker);
	
	char *bus_width = grabHex(block_data_buff, 16, 1);
	if (bus_width[0] == 0x00){
		sprintf(value, "Bus Width(0x10):\t\t\t1 bit Width");
	}
	else if (bus_width[0] == 0x10){
		sprintf(value, "Bus Width(0x10):\t\t\t4 bit Width");
	}
	printf("\n%s", value);
	free(bus_width);
	
	char *secure_mode = grabHex(block_data_buff, 17, 1);
	if (secure_mode[0] == 0x00){
		sprintf(value, "Secured Mode(0x11):\t\t\tNot in the secured mode");
	}
	else if (secure_mode[0] == 0x01){
		sprintf(value, "Secured Mode(0x11):\t\t\tIn secured mode");
	}
	printf("\n%s", value);
	free(secure_mode);
	
	char *speed_class = grabHex(block_data_buff, 18, 1);
	if (speed_class[0] == 0x00){
		sprintf(value, "Speed Class(0x12):\t\t\tClass 0");
	}
	else if (speed_class[0] == 0x01){
		sprintf(value, "Speed Class(0x12):\t\t\tClass 2");
	}
	else if (speed_class[0] == 0x02){
		sprintf(value, "Speed Class(0x12):\t\t\tClass 4");
	}
	else if (speed_class[0] == 0x03){
		sprintf(value, "Speed Class(0x12):\t\t\tClass 6");
	}
	else if (speed_class[0] == 0x04){
		sprintf(value, "Speed Class(0x12):\t\t\tClass 10");
	}
	printf("\n%s", value);
	free(speed_class);
	
	char *uhs_speed_grade = grabHex(block_data_buff, 19, 1);
	if (uhs_speed_grade[0] == 0x00){
		sprintf(value, "UHS Speed Grade(0x13):\t\t\tLess than 10MB/s");
	}
	else if (uhs_speed_grade[0] == 0x01){
		sprintf(value, "UHS Speed Grade(0x13):\t\t\t10MB/s and above");
	}
	else if (uhs_speed_grade[0] == 0x03){
		sprintf(value, "UHS Speed Grade(0x13):\t\t\t30MB/s and above");
	}
	printf("\n%s", value);
	free(uhs_speed_grade);
	
	double rttb_count = hexArrToDec(block_data_buff, 26, 1);
	sprintf(value, "New Bad block Count(0x1A):\t\t%.0f", rttb_count);
	printf("\n%s", value);
	
	double spare_block = hexArrToDec(block_data_buff, 30, 2);
	sprintf(value, "Spare Block(0x1E):\t\t\t%.0f", spare_block);
	printf("\n%s", value);
	
	double min_erase_count_mt = hexArrToDec(block_data_buff, 32, 4);
	sprintf(value, "Min Erase Count(0x20):\t\t\t%.0f", min_erase_count_mt);
	printf("\n%s", value);
	
	double max_erase_count_mt = hexArrToDec(block_data_buff, 36, 4);
	sprintf(value, "Max Erase Count(0x24):\t\t\t%.0f", max_erase_count_mt);
	printf("\n%s", value);
	
	double total_erase_count_mt = hexArrToDec(block_data_buff, 40, 4);
	sprintf(value, "Total Erase Count(0x28):\t\t%.0f", total_erase_count_mt);
	printf("\n%s", value);
	
	double avg_erase_count_mt = hexArrToDec(block_data_buff, 44, 4);
	sprintf(value, "Avg. Erase Count(0x2C):\t\t\t%.0f", avg_erase_count_mt);
	printf("\n%s", value);
	
	double nand_pe_cycle = hexArrToDec(block_data_buff, 68, 2);
	sprintf(value, "NAND P/E Cycle(0x44):\t\t\t%.0f", nand_pe_cycle * 100);
	printf("\n%s", value);
	
	sprintf(value, "Card Life(0x46):\t\t\t%d %%", block_data_buff[70]);
	printf("\n%s", value);
	
	char *speed_mode = grabHex(block_data_buff, 71, 1);
	if (speed_mode[0] == 0x00){
		sprintf(value, "Current SD Card Speed Mode(0x47):\tDefault speed");
	}
	else if (speed_mode[0] == 0x01){
		sprintf(value, "Current SD Card Speed Mode(0x47):\tHigh speed mode");
	}
	else if (speed_mode[0] == 0x10){
		sprintf(value, "Current SD Card Speed Mode(0x47):\tSDR12");
	}
	else if (speed_mode[0] == 0x11){
		sprintf(value, "Current SD Card Speed Mode(0x47):\tSDR25");
	}
	else if (speed_mode[0] == 0x12){
		sprintf(value, "Current SD Card Speed Mode(0x47):\tSDR50");
	}
	else if (speed_mode[0] == 0x14){
		sprintf(value, "Current SD Card Speed Mode(0x47):\tDDR50");
	}
	else if (speed_mode[0] == 0x18){
		sprintf(value, "Current SD Card Speed Mode(0x47):\tSDR104");
	}
	printf("\n%s", value);
	free(speed_mode);
	
	double total_write_crc_count = hexArrToDec(block_data_buff, 72, 4);
	sprintf(value, "Total Write CRC Count(0x48):\t\t%.0f", total_write_crc_count);
	printf("\n%s", value);
	
	double power_on_off_count = hexArrToDec(block_data_buff, 76, 4);
	sprintf(value, "Power On/Off Count(0x4C):\t\t%.0f", power_on_off_count);
	printf("\n%s", value);

	char *nand_flash_id = grabHex(block_data_buff, 80, 6);
	sprintf(value, "NAND Flash ID(0x50):\t\t\t%02X-%02X-%02X-%02X-%02X-%02X", (unsigned char)nand_flash_id[0], (unsigned char)nand_flash_id[1], (unsigned char)nand_flash_id[2], (unsigned char)nand_flash_id[3], (unsigned char)nand_flash_id[4], (unsigned char)nand_flash_id[5]);
	printf("\n%s", value);
	free(nand_flash_id);
	
	
	char *smi_sd_controller_pn = grabString(block_data_buff, 88, 8);
	sprintf(value, "SMI SD Controller P/N(0x58):\t\t%s", smi_sd_controller_pn);
	printf("\n%s", value);
	free(smi_sd_controller_pn);
	
	char *sd_firmware_version = grabString(block_data_buff, 128, 6);
	sprintf(value, "SD Firmware Version(0x80):\t\t%s", sd_firmware_version);
	printf("\n%s\n", value);
	free(sd_firmware_version);
	
	return;
}

void parsing_Health_info(char *block_data_buff)
{
	char value[64];
	sprintf(value, "Card Life(0x46):\t\t\t%d %%", block_data_buff[70]);
	printf("\n%s\n", value);
	return;
}


char* grabString(char* data, int start_pos, int length){
	char *str = (char*)malloc( (length + 1) * sizeof(char) );
	strncpy(str, data + start_pos, length);
	str[length] = '\0';
	return str;
}

char* grabHex(char* data, int start_pos, int length){
	char *str = (char*)malloc( (length) * sizeof(char) );
	memcpy(str, data + start_pos, length);
	return str;
}

double hexArrToDec(char *data, int start_pos, int length){
	double tmp = 0;
	for(int i = 0; i < length; i++){
		double shift = (unsigned char)data[start_pos+i] << (4* (2 * ((length-i)-1))); 
		tmp += shift;
	}
	return tmp;
}

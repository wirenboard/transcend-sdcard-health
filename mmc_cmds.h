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

/* mmc_cmds.c */

int do_write_extcsd(int nargs, char **argv);
int do_writeprotect_boot_get(int nargs, char **argv);
int do_writeprotect_boot_set(int nargs, char **argv);
int do_writeprotect_user_get(int nargs, char **argv);
int do_writeprotect_user_set(int nargs, char **argv);
int do_disable_512B_emulation(int nargs, char **argv);
int do_write_boot_en(int nargs, char **argv);
int do_boot_bus_conditions_set(int nargs, char **argv);
int do_write_bkops_en(int nargs, char **argv);
int do_hwreset_en(int nargs, char **argv);
int do_hwreset_dis(int nargs, char **argv);
int do_sanitize(int nargs, char **argv);
int do_status_get(int nargs, char **argv);
int do_create_gp_partition(int nargs, char **argv);
int do_enh_area_set(int nargs, char **argv);
int do_write_reliability_set(int nargs, char **argv);
int do_rpmb_write_key(int nargs, char **argv);
int do_rpmb_read_counter(int nargs, char **argv);
int do_rpmb_read_block(int nargs, char **argv);
int do_rpmb_write_block(int nargs, char **argv);
int do_cache_en(int nargs, char **argv);
int do_cache_dis(int nargs, char **argv);
int do_ffu(int nargs, char **argv);
int do_read_scr(int argc, char **argv);
int do_read_cid(int argc, char **argv);
int do_read_csd(int argc, char **argv);

/* SD SMART */
int do_SMART_buffer_dump(int nargs, char **argv); //Show SMART raw buffer
int show_SMART_info(int nargs, char **argv); //Show SMART info
int show_Health_info(int nargs, char **argv); //Show Health info
int CMD56_data_in(int fd, int argCmd56, char *block_data_buff);
void dump_smart_data(char *block_data_buff);
void is_transcend_card(char *block_data_buff, char function);
void parsing_SMART_info(char *block_data_buff);
void parsing_Health_info(char *block_data_buff);
char *grabString(char* data, int start_pos, int length);
char *grabHex(char* data, int start_pos, int length);
double hexArrToDec(char *data, int start_pos, int length);

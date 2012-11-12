/****************************************************************************
 *       Filename:  map_test.c
 *
 *    Description:  test for map.c
 *
 *        Version:  1.0
 *        Created:  10/23/2012 01:26:28 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <check.h>

#include "map.h"
#include "error.h"
#include "debug.h"

START_TEST(test_invalid_init)
{
    int ret;
    int client_x_num, client_y_num, server_amount, comm = 1;
    double server_ratio = 0.1;
    int best_server_amount;

    client_x_num = 17; client_y_num = 17; server_amount = 29;
    best_server_amount = (int)((double)client_x_num * (double)client_y_num 
	    * server_ratio);
    ret = iofw_map_init(client_x_num, client_y_num, 
	    server_amount, best_server_amount, comm);
    fail_unless(ret == IOFW_ERROR_INVALID_INIT_ARG,
	    "(%d*%d, %d) and best(%d) for map init should be invalid", 
	    client_x_num, client_y_num, server_amount, best_server_amount);

    client_x_num = 30; client_y_num = 30; server_amount = 80;
    best_server_amount = (int)((double)client_x_num * (double)client_y_num 
	    * server_ratio);
    ret = iofw_map_init(client_x_num, client_y_num, 
	    server_amount, best_server_amount, comm);
    fail_unless(ret == IOFW_ERROR_INVALID_INIT_ARG,
	    "(%d*%d, %d) and best(%d) for map init should be invalid", 
	    client_x_num, client_y_num, server_amount, best_server_amount);
}
END_TEST

START_TEST(test_valid_init)
{
    int ret;
    int client_x_num, client_y_num, server_amount, comm = 1;
    double server_ratio = 0.1;
    int best_server_amount;
    int _server_amount, _client_amount;
    int proc_type;
    int i;
    int client_id[] = {0, 899}, server_id[] = {900, 989};
    int blank_id[] = {990, 998};

    client_x_num = 30; client_y_num = 30; server_amount = 90;
    best_server_amount = (int)((double)client_x_num * (double)client_y_num 
	    * server_ratio);
    ret = iofw_map_init(client_x_num, client_y_num, 
	    server_amount, best_server_amount, comm);
    fail_unless(ret == IOFW_ERROR_NONE,
	    "(%d*%d, %d) and best(%d) for map init should be valid", 
	    client_x_num, client_y_num, server_amount, best_server_amount);

    _server_amount = iofw_map_get_server_amount();
    fail_unless(_server_amount == server_amount, 
	    "Server amount = %d, but should = %d", _server_amount, server_amount);
    
    _client_amount = iofw_map_get_client_amount();
    fail_unless(_client_amount == client_x_num * client_y_num,
	    "Client_amount = %d, but should = %d", _client_amount, 
	    client_x_num * client_y_num);

    for(i = 0; i < sizeof(client_id) / sizeof(int); i ++)
    {
	proc_type = iofw_map_proc_type(client_id[i]);
	fail_unless(proc_type == IOFW_MAP_TYPE_CLIENT, 
		"Proc %d should be client", client_id[0]);
    }
    for(i = 0; i < sizeof(server_id) / sizeof(int); i ++)
    {
	proc_type = iofw_map_proc_type(server_id[i]);
	fail_unless(proc_type == IOFW_MAP_TYPE_SERVER, 
		"Proc %d should be server", server_id[0]);
    }
    for(i = 0; i < sizeof(blank_id) / sizeof(int); i ++)
    {
	proc_type = iofw_map_proc_type(blank_id[i]);
	fail_unless(proc_type == IOFW_MAP_TYPE_BLANK,
		"Proc %d should be server", blank_id[0]);
    }
}
END_TEST

/* start more proc than needed */
START_TEST(test_overflow_server_init)
{
    int ret;
    int client_x_num, client_y_num, server_amount, comm = 1;
    double server_ratio = 0.1;
    int best_server_amount;
    int _server_amount, _client_amount;
    int proc_type;
    int i;
    int client_id[] = {0, 899}, server_id[] = {900, 989};
    int blank_id[] = {990, 998};

    client_x_num = 30; client_y_num = 30; server_amount = 100;
    best_server_amount = (int)((double)client_x_num * (double)client_y_num 
	    * server_ratio);
    ret = iofw_map_init(client_x_num, client_y_num, 
	    server_amount, best_server_amount, comm);
    fail_unless(ret == IOFW_ERROR_NONE,
	    "(%d*%d, %d) and best(%d) for map init should be valid", 
	    client_x_num, client_y_num, server_amount, best_server_amount);
    server_amount = 90; //assign for later check

    _server_amount = iofw_map_get_server_amount();
    fail_unless(_server_amount == server_amount, 
	    "Server amount = %d, but should = %d", _server_amount, server_amount);
    
    _client_amount = iofw_map_get_client_amount();
    fail_unless(_client_amount == client_x_num * client_y_num,
	    "Client_amount = %d, but should = %d", _client_amount, 
	    client_x_num * client_y_num);

    for(i = 0; i < sizeof(client_id) / sizeof(int); i ++)
    {
	proc_type = iofw_map_proc_type(client_id[i]);
	fail_unless(proc_type == IOFW_MAP_TYPE_CLIENT, 
		"Proc %d should be client", client_id[0]);
    }
    for(i = 0; i < sizeof(server_id) / sizeof(int); i ++)
    {
	proc_type = iofw_map_proc_type(server_id[i]);
	fail_unless(proc_type == IOFW_MAP_TYPE_SERVER, 
		"Proc %d should be server", server_id[0]);
    }
    for(i = 0; i < sizeof(blank_id) / sizeof(int); i ++)
    {
	proc_type = iofw_map_proc_type(blank_id[i]);
	fail_unless(proc_type == IOFW_MAP_TYPE_BLANK,
		"Proc %d should be blank", blank_id[0]);
    }
}
END_TEST

START_TEST(test_client_num_of_server)
{
    int ret, i;
    int client_x_num, client_y_num, server_amount, comm = 1;
    double server_ratio = 0.1;
    int best_server_amount;
    int server_id[] = {900, 913, 989};
    int should_client_num, client_num;

    client_x_num = 30; client_y_num = 30; server_amount = 90;
    should_client_num = 10;
    best_server_amount = (int)((double)client_x_num * (double)client_y_num 
	    * server_ratio);
    ret = iofw_map_init(client_x_num, client_y_num, 
	    server_amount, best_server_amount, comm);
    
    for(i = 0; i < sizeof(server_id) / sizeof(int); i ++)
    {
	client_num = iofw_map_get_client_num_of_server(server_id[i]);
	fail_unless(client_num == 10, "Client number of Server %d is %d, but"
		"should be %d.", server_id[i], client_num, should_client_num);
    }
}
END_TEST

START_TEST(test_client_index_of_server_case1)
{
    int ret, i;
    int client_x_num, client_y_num, server_amount, comm = 1;
    double server_ratio = 0.1;
    int best_server_amount;
    int client_id[] = {0, 215, 400, 666, 899};
    int should_client_index[] = {0, 5, 0, 6, 9}; 
    int client_index;

    client_x_num = 30; client_y_num = 30; server_amount = 90;
    best_server_amount = (int)((double)client_x_num * (double)client_y_num 
	    * server_ratio);
    ret = iofw_map_init(client_x_num, client_y_num, 
	    server_amount, best_server_amount, comm);
    
    for(i = 0; i < sizeof(client_id) / sizeof(int); i ++)
    {
	client_index = iofw_map_get_client_index_of_server(client_id[i]);
	fail_unless(client_index == should_client_index[i], 
		"Index of Client %d is %d, but should be %d.", 
		client_id[i], client_index, should_client_index[i]);
    }
}
END_TEST

START_TEST(test_client_index_of_server_case2)
{
    int ret, i;
    int client_x_num, client_y_num, server_amount, comm = 1;
    double server_ratio = 0.1;
    int best_server_amount;
    int client_id[] = {0, 32, 70, 127, 149};
    int should_client_index[] = {0, 2, 0, 7, 9}; 
    int client_index;

    client_x_num = 30; client_y_num = 5; server_amount = 15;
    best_server_amount = (int)((double)client_x_num * (double)client_y_num 
	    * server_ratio);
    ret = iofw_map_init(client_x_num, client_y_num, 
	    server_amount, best_server_amount, comm);
    
    for(i = 0; i < sizeof(client_id) / sizeof(int); i ++)
    {
	client_index = iofw_map_get_client_index_of_server(client_id[i]);
	fail_unless(client_index == should_client_index[i], 
		"Index of Client %d is %d, but should be %d.", 
		client_id[i], client_index, should_client_index[i]);
    }
}
END_TEST

START_TEST(test_forwarding_case1)
{
    int client_x_num, client_y_num, server_amount, comm = 1;
    double server_ratio = 0.1;
    int best_server_amount;
    int i;
    iofw_msg_t msg;
    int client_id[] = {0, 215, 400, 666, 899};
    int server_id[] = {900, 921, 940, 966, 989};

    client_x_num = 30; client_y_num = 30; server_amount = 90;
    best_server_amount = (int)((double)client_x_num * (double)client_y_num 
	    * server_ratio);
    iofw_map_init(client_x_num, client_y_num, 
	    server_amount, best_server_amount, comm);

    for(i = 0; i < sizeof(client_id) / sizeof(int); i ++)
    {
	msg.src = client_id[i];
	iofw_map_forwarding(&msg);
	fail_unless(msg.dst == server_id[i], "client %d should send msg to "
		"server %d, but send to server %d", client_id[i], server_id[i],
		msg.dst);
    }
}
END_TEST

START_TEST(test_forwarding_case2)
{
    int client_x_num, client_y_num, server_amount, comm = 1;
    double server_ratio = 0.1;
    int best_server_amount;
    int i;
    iofw_msg_t msg;
    int client_id[] = {0, 32, 70, 127, 149};
    int server_id[] = {150, 153, 157, 162, 164};

    client_x_num = 30; client_y_num = 5; server_amount = 15;
    best_server_amount = (int)((double)client_x_num * (double)client_y_num 
	    * server_ratio);
    iofw_map_init(client_x_num, client_y_num, 
	    server_amount, best_server_amount, comm);

    for(i = 0; i < sizeof(client_id) / sizeof(int); i ++)
    {
	msg.src = client_id[i];
	iofw_map_forwarding(&msg);
	fail_unless(msg.dst == server_id[i], "client %d should send msg to "
		"server %d, but send to server %d", client_id[i], server_id[i],
		msg.dst);
    }
}
END_TEST

Suite* map_suite()
{
    Suite *s = suite_create("Map");

    TCase *tc_invalid = tcase_create("Invalid");
    tcase_add_test(tc_invalid, test_invalid_init);
    TCase *tc_valid = tcase_create("Valid");
    tcase_add_test(tc_valid, test_valid_init);
    tcase_add_test(tc_valid, test_overflow_server_init);
    TCase *tc_api = tcase_create("API");
    tcase_add_test(tc_api, test_client_num_of_server);
    tcase_add_test(tc_api, test_client_index_of_server_case1);
    tcase_add_test(tc_api, test_client_index_of_server_case2);
    tcase_add_test(tc_api, test_forwarding_case1);
    tcase_add_test(tc_api, test_forwarding_case2);
    suite_add_tcase(s, tc_invalid);
    suite_add_tcase(s, tc_valid);
    suite_add_tcase(s, tc_api);

    return s;
}

int main()
{
    int num_failed;

    set_debug_mask(DEBUG_MAP);
    Suite *s = map_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    num_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


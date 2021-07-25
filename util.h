#include <stdio.h>
#include <stdbool.h>

struct user_info
{
	char username[50];
	char password[50];
	int status;//1 for active,0 for inactive
	int type;//1 for admin,2 for normal user
	char acc_num[512];
	bool session_var;
	
};

struct account_info
{
	char acc_num[512];
	double balance;
	int status;//1 for active,0 for inactive
	bool joint_acc;
};

struct transaction_info
{
	char acc_num[512];
	char date[50];
	char t_type[10];//credit or debit
	double amount;
	double avl_bal;
};
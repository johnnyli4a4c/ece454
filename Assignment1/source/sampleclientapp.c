#include <stdio.h>
#include "ece454rpc_types.h"

int main()
{
    int a = -10, b = 20;
    return_type ans = make_remote_call("ecelinux5.uwaterloo.ca",
	                               10000,
				       "addtwo", 2,
	                               sizeof(int), (void *)(&a),
	                               sizeof(int), (void *)(&b));
    int i = *(int *)(ans.return_val);
    printf("client, got result: %d\n", i);

    int c = 4, d = 2;
    return_type ans2 = make_remote_call("ecelinux5.uwaterloo.ca",
	                               10000,
				       "minustwo", 2,
	                               sizeof(int), (void *)(&c),
	                               sizeof(int), (void *)(&d));
    int j = *(int *)(ans2.return_val);
    printf("client, got result: %d\n", j);

    return 0;
}

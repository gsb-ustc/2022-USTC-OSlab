#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/syscall.h>
struct ps_info
{
	char Command[20];
	//float Cpu;
};
int main(void)
{
    while(1)
    {
        struct ps_info Ps1[100],Ps2[100];
        char commands[1000];
        char commands2[1000];
        pid_t Pid[100];
        pid_t Pid2[100];
        long State[100];
         long State2[100];
        int Cpu1[100];
        int Cpu2[100];
        syscall(332, Pid);
        syscall(333, commands);
        syscall(334, State);
        syscall(335,Cpu1);
        sleep(1);
        syscall(332, Pid2);
        syscall(333, commands2);
        syscall(334, State2);
        syscall(335,Cpu2);
        for(int i = 0; i < 100; ++i)
        {
            for(int j = 0; j < 16; ++j)
            {
                if(commands[i*16 + j] != '\0')  Ps1[i].Command[j] = commands[i*16+j];
                else if(j != 15)
                {
                    Ps1[i].Command[j] = ' ';
                }
                else Ps1[i].Command[j] = '\0';
            }
        }
        for(int i = 0; i < 100; ++i)
        {
            for(int j = 0; j < 16; ++j)
            {
                if(commands2[i*16 + j] != '\0')  Ps2[i].Command[j] = commands2[i*16+j];
                else if(j != 15)
                {
                    Ps2[i].Command[j] = ' ';
                }
                else Ps2[i].Command[j] = '\0';
            }
        }
        float Cpu3[100];
        for(int i = 0; i < 50; ++i)
        {
            for(int j = 0; j < 100; ++j)
            {
                if(strcmp(Ps1[i].Command,Ps2[j].Command) == 0)  
                {    
                    State[i] = State2[j];
                    Cpu3[i] = (Cpu2[j] - Cpu1[i]) / 10000000.0;
                    break;
                }
            }
        }
        int n = 100;
        for(int i=0; i<n-1; i++)
        {

            int max_index = i; 
            for(int j=i+1; j<n; j++)
            {
                if(Cpu3[j] > Cpu3[max_index])
                {
                   max_index = j;
                }
            }

            if( i != max_index)
            {
                int temp1 = Cpu3[i];
                pid_t temp2 = Pid[i];
                long temp3 = State[i];
                Cpu3[i] = Cpu3[max_index];
                Cpu3[max_index] = temp1;
                Pid[i] = Pid[max_index];
                Pid[max_index] = temp2;
                State[i] = State[max_index];
                State[max_index] = temp3;
                for(int k = 0; k < 16; ++k)
                {
                    char temp4;
                    temp4 = Ps1[i].Command[k];
                    Ps1[i].Command[k] = Ps1[max_index].Command[k];
                    Ps1[max_index].Command[k] = temp4;
                }

            }
        }



        printf("PID\tCOMM \t CPU\tISRUNNING\n");
        for(int j = 0; j < 20; ++j)
        {
            printf("%d\t", Pid[j]);
            printf("%s\t\t",Ps1[j].Command);
            printf("%lf%%\t\t",Cpu3[j]);
            printf("%ld\n",State[j]);
        }
        //sleep(1);
        //system("clear");
        //sleep(1);
        //system("clear");
    }
    return 0;
}
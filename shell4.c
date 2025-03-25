// name : Isabelle Champion ID: 260970545
// Assumptions:
// - fg with no argument returns the 0th Job
// when doing redirects command > file, when creating a new file (if doesn't exist), give it write permissions

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

char* line = NULL;
struct Node{
        int process_id;
        char* command;
        struct Node* next;
        struct Node* prev;
};

struct Node* head; 


int getcmd(char *prompt, char *args[], int *background ){

        for(int i = 0; args[i] != NULL ; i ++){
                args[i] = NULL;
        }
        int length, i = 0;
        char *token, *loc;
        size_t linecap = 0;

        printf("%s", prompt);
        length = getline(&line, &linecap, stdin);
        if(length <= 0){
		// deallocate memory
		free(line);

		// free linked list and terminate running processes
		struct Node* h = head; struct Node* m = head;
		while(h!= NULL){
			kill(h->process_id,15);
			m = h->next;
			free(h);
			h = m;
		}
		
                exit(-1);
        }
	if(length == 1 && line[0] == '\n'){
		//error: entered an empty string
		args[0] = "";
	}
	else{

	// Check if background is specified.. 
        	if ((loc = index(line, '&')) != NULL) {
                	*background = 1;
                	*loc = ' ';
        	} else
                	*background = 0;

        	while ((token = strsep(&line, " \t\n")) != NULL) {
                	for (int j = 0; j < strlen(token); j++)
                        	if (token[j] <= 32) 
                                	token[j] = '\0';
                	if (strlen(token) > 0)
                        	args[i++] = token;
        	}
	}

        return i;
}


int main(void){

        char *args[20];
        int bg;
        pid_t pid;
	
	head = (struct Node*)malloc(sizeof(struct Node));

	head->next = NULL; head->prev = NULL;
	while(1){
		bg = 0;
		int cnt = getcmd("\n>> ", args, &bg);
		

		if(strcmp(args[0], "pwd") == 0){
			char buf[1000];
			getcwd(buf, sizeof(buf));
			printf("%s\n",buf);

        	}	
		else if(strcmp(args[0], "echo") == 0){
			
			for(int i = 1 ; i < cnt ; i ++){
				printf("%s ", args[i]);
			}
        	}
		else if(strcmp(args[0], "cd") == 0){
			chdir(args[1]);	
		}
		else if(strcmp(args[0], "jobs") == 0){
			int j = 0;
			struct Node* cur = head->next;
			int ret_status;
				
			while(cur != NULL){
				// still running
				if (waitpid(cur->process_id,&ret_status,WNOHANG) == 0){
    	                        	printf("%d : %s\n",j, cur->command);
					cur = cur->next; j++; 
				}
				else{// not running
					cur = cur->next; 
				} 
			}
		}
		else if(strcmp(args[0], "fg") == 0){
			
			struct Node* cur = head->next; 
			int count = 0; int num = 0; 
			struct Node* cur2;
			
			if(args[1] != NULL){
				num = atoi(args[1]);
			}
			int ret_status;
			
			while(cur != NULL){
				if(waitpid(cur->process_id,&ret_status,WNOHANG) != 0){
					// not running
					// take out of linked list
					cur2 = cur->next;
					if(cur2 != NULL){
						cur->prev->next = cur->next;
						cur->next->prev = cur->prev;
					}
					else{
						cur->prev->next = NULL;
					}
					cur = cur2;
				}
				else{
					if (count == num){
						// wait for process
						int ret_status;
						waitpid(cur->process_id, &ret_status, 0);

						cur2 = cur ->next;
						if(cur2 != NULL){
							cur->prev->next = cur->next;
							cur->next->prev = cur->prev;
						}
						else{
							cur->prev->next = NULL;
						}	
						cur = cur2;
						break;
					}
					cur = cur->next;
					count++;
					
				}	
			}	
		}

		else if(strcmp(args[0], "exit") == 0){
			
			struct Node* i = head; struct Node* j = head;
			while(i->next != NULL){
				j = i;
				kill(i->process_id,15);
				i= i->next;
				free(j);
			}
			free(line); // free the line
			exit(0);	
			
		}
	
		else {

                	pid = fork();

                	if (pid < 0){
                        	printf("error");
                	}
                	if(pid == 0){
    				int red_pip=0; int fail = 0;
   	   	                for(int i = 0; i < cnt ; i ++){
                       			if(strcmp(args[i], ">") == 0){ // output of command (LHS) to file (RHS)
						
						red_pip = 1; 
						close(1);
						int fd = open(args[i+1], O_WRONLY | O_CREAT, 0666);
						if(fd < 0){
							printf("%d\n : error", fd);
						}
						char *newer_args[20];
						
						
						for(int j = 0 ; j < i; j++){
							newer_args[j] = args[j];
						
						}newer_args[i] = NULL;
						execvp(newer_args[0],newer_args); exit(0);
						printf("exec failed");	

                       			}
					if(strcmp(args[i], "<") == 0){ // input to command (LHS) from file (RHS)
						// red_pip: 1 if a redirect or a pipe	
						red_pip = 1;
						close(0);
						int fd = open(args[i+1], O_RDONLY);
						if(fd < 0){
							printf("%d\n : error", fd);
						}
						char *newer_args[20];
						for(int j = 0 ; j < i; j++){
							newer_args[j] = args[j];
						}newer_args[i] = NULL;
						execvp(newer_args[0],newer_args); exit(0);
						printf("exec failed");

					}  
                       			if(strcmp(args[i], "|") == 0){ 
                   				int f[2];

						pipe(f);
				
						pid_t pid2 = fork();					
						if(pid2 != 0){ // parent
							close(1);
							dup(f[1]);
							close(f[0]);

							char *new_args[i+1];
							for(int j = 0 ; j < i; j++){
								new_args[j] = args[j];
							}new_args[i] = NULL;
							execvp(new_args[0],new_args);
							printf("pipe parent exec failed"); exit(0);
							
						}
						else{ // child
							close(0);
							dup(f[0]);
							close(f[1]);
							
							char *new_args[i+1]; int k = 0;
							for(int j = i+1 ; j < cnt; j++){
								new_args[k] = args[j]; k++;
							} new_args[k] = NULL;
					
							execvp(new_args[0],new_args);
							printf("pipe child exec failed"); exit(0);
						}
					
                       			}   
                		}
				if(red_pip == 0){
					execvp(args[0],args); exit(0);
				}
			}
			else{ // parent add child to linked list if & (after forking)
				
				int ret_status;
				if(bg == 1){ 
					// add to linked list of jobs
					struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
					newNode->process_id = pid;
				
                                	char* total = (char*)malloc(cnt +1);
                                	for(int i = 0; i < cnt ; i++){
                                        	strcat(total,args[i]);
                                        	strcat(total," ");
                                	}
					newNode->command = total;
						
					struct Node* pointer = head;
					while(pointer->next!= NULL){
						pointer = pointer->next;
                                	}
					pointer->next = newNode; 
					//newNode->next = NULL;
					newNode->prev = pointer;
				}else{

					waitpid(pid, &ret_status,0);
				}
			}
		}
	}

}
	


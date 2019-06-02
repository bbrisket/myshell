#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

typedef struct redirectTok redirectTok;
struct redirectTok {
	char* currTok;
	redirectTok* next;
};

typedef struct word word;
struct word {
	redirectTok* currRTok;
	word* next;
};

typedef struct phrase phrase;
struct phrase {
	word* currWord;
	phrase* next;
};

void myPrint(char *msg){
	write(STDOUT_FILENO, msg, strlen(msg));
}

void printError(){
	char error_message[30] = "An error has occurred\n";
	write(STDOUT_FILENO, error_message, strlen(error_message));
}

redirectTok* parseRTok(char* input_token){
	char* str;
	char* token = strtok_r(input_token, " \n\t", &str);
	if (token != NULL){
		redirectTok* newRTok = malloc(sizeof(redirectTok));
		newRTok -> currTok = strdup(token);
		newRTok -> next = NULL;
		redirectTok* front = newRTok;

		token = strtok_r(NULL, " \n\t", &str);
		while (token != NULL){
			redirectTok* temp = malloc(sizeof(redirectTok));
			temp -> currTok = strdup(token);
			temp -> next = NULL;

			newRTok -> next = temp;
			newRTok = newRTok -> next;
			token = strtok_r(NULL, " \n\t", &str);
		}

		return front;
	}
	return NULL;
}

word* parseWord(char* input_token){
	char* str;
	char* token = strtok_r(input_token, ">", &str);

	word* new_word = malloc(sizeof(word));
	word* front = new_word;

	redirectTok* temp = parseRTok(token);
	new_word -> currRTok = temp;
	new_word -> next = NULL;

	token = strtok_r(NULL, ">", &str);
	while (token != NULL){
		temp = parseRTok(token);

		word* temp_word = malloc(sizeof(word));
		temp_word -> currRTok = temp;
		temp_word -> next = NULL;
		new_word -> next = temp_word;

		token = strtok_r(NULL, ">", &str);
		new_word = new_word -> next;
	}
	return front;
}

phrase* parsePhrase(char* sourceFile){
	char* str;
	char* token = strtok_r(sourceFile, ";", &str);

	phrase* new_phrase = malloc(sizeof(phrase));
	phrase* front = new_phrase;

	word* temp = parseWord(token);
	new_phrase -> currWord = temp;
	new_phrase -> next = NULL;

	token = strtok_r(NULL, ";", &str);
	while (token != NULL){
		temp = parseWord(token);

		phrase* temp_phrase = malloc(sizeof(phrase));
		temp_phrase -> currWord = temp;
		temp_phrase -> next = NULL;
		new_phrase -> next = temp_phrase;

		token = strtok_r(NULL, ";", &str);
		new_phrase = new_phrase -> next;
	}
	return front;
}

void executeExit(redirectTok* rToken, int redirect){
	int len = 0;
	while (rToken != NULL){
        	len++;
        	rToken = rToken -> next;
	}

	if (len == 1 && redirect != 1){
        	exit(0);
	} else {
        	printError();
	}
}

void executeCd(redirectTok* rToken, int redirect){
	if (redirect == 1){
		printError();
		return;
	}

	char* path;
	char* token = rToken -> currTok;
	if (strcmp("cd", token) == 0){
		if (rToken -> next == NULL){
	        	path = getenv("HOME");
        	} else {
			rToken = rToken -> next;
			if (rToken -> next != NULL){
				printError();
                		return;
        		}
        		path = rToken -> currTok;
        	}

		int flag;
        	if ((flag = chdir(path)) == 0){
			return;
        	} else {
			printError();
			return;
        	}
	} else {
		printError();
	}
}

void executePwd(redirectTok* rToken, int redirect, word* targetWord){
	int len = 0;
	while (rToken != NULL){
		len++;
		rToken = rToken -> next;
	}

	if (len == 1){
		if (redirect == 1){
			printError();
			return;
		}

		char a[1023];
		getcwd(a, 1023);

		char b[1024];
		sprintf(b, "%s\n", a);

		int s;
		if (redirect == 1){
			int r = open(targetWord -> next -> currRTok -> currTok, O_CREAT | O_RDWR);
			if (r == -1){
				printError();
				return;
			}
			s = dup(1);
			dup2(r, 1);
			close(r);
		}

		myPrint(b);
		if (redirect == 1){
			close(1);
			dup2(s, 1);
			close(s);
		}
	} else {
		printError();
	}
}

void executeCommand(redirectTok* rToken, int redirection, word* targetWord){
	int child;
	pid_t childPid;

	int len = 1;
	redirectTok* front = rToken;
	while (rToken != NULL){
		len++;
		rToken = rToken -> next;
	}
	rToken = front;

	char* valList[len];
	int i = 0;
	while (rToken != NULL){
		valList[i] = rToken -> currTok;
		i++;
		rToken = rToken -> next;
	}
	valList[i] = NULL;

	int s;
	if ((childPid = fork()) < 0){
		printError();
	} else if (childPid == 0){
		if (redirection == 1){
			int r = open(targetWord -> next -> currRTok -> currTok, O_RDWR | O_CREAT, 0664);
			if (r == -1){
				printError();
				exit(0);
			}

			s = dup(1);
			dup2(r, 1);
			close(r);
		}
		if (execvp(valList[0], valList) < 0){
			printError();
		}
		if (redirection == 1){
			close(1);
			dup2(s, 1);
			close(s);
		}
		exit(0);
	} else {
		while (wait(&child) != childPid){
			continue;
		}
	}
}

void executeAll(phrase* targetPhrase){
	while (targetPhrase != NULL){
		word* targetWord = targetPhrase -> currWord;

		redirectTok* rToken;
		char* token;

		int redirect = -1;
		if (targetWord -> next == NULL){
			redirect = 0;
		} else {
			word* nextWord = targetWord -> next;
			if (nextWord -> next == NULL){
				rToken = nextWord -> currRTok;
				if (rToken != NULL && rToken -> next == NULL){
					token = rToken -> currTok;
					if (access(token, F_OK) == -1){
						redirect = 1;
					}
				}
			}
		}

		if (redirect == -1) {
			targetPhrase = targetPhrase -> next;
			printError();
			continue;
		}

		rToken = targetWord -> currRTok;
		if (rToken != NULL){
			token = rToken -> currTok;
			if (strcmp("cd", token) == 0){
				executeCd(rToken, redirect);
			} else if (strcmp("pwd", token) == 0){
				executePwd(rToken, redirect, targetWord);
			} else if (strcmp("exit", token) == 0){
				executeExit(rToken, redirect);
			} else {
				executeCommand(rToken, redirect, targetWord);
			}
		}
		targetPhrase = targetPhrase -> next;
	}
}

int isWhitespace(int c){
	int flag = 0;
	if  (c == ' '){
		flag = 1;
	} else if (c == '\t'){
		flag = 1;
	} else if (c == '\n'){
		flag = 1;
	} else if (c == '\r'){
		flag = 1;
	} else if (c == '\v'){
		flag = 1;
	} else if (c == '\f'){
		flag = 1;
	}
	return flag;
}

int main(int argc, char *argv[]){
	FILE *sourceFile;
	char cmd_buff[514];
	char *pinput;
	int batchFlag;

	if (argc == 1){
		sourceFile = stdin;
		batchFlag = 0;
	} else if (argc == 2){
		sourceFile = fopen(argv[1], "r");
		if (sourceFile == NULL){
			printError();
			exit(1);
		}
		batchFlag = 1;
	} else {
		printError();
		exit(1);
	}

	while (1){
		if (!batchFlag){
			myPrint("myshell> ");
		}

		pinput = fgets(cmd_buff, 513, sourceFile);
		if (!pinput){
			exit(0);
		}

		int len = 0;
		while (len<513 && pinput[len] != '\n'){
			len++;
		}

		if (!(len != 513 || pinput[len] == '\n')){
			myPrint(pinput);
			char c;
			while ((c = fgetc(sourceFile))){
				myPrint(&c);
				if (c == '\n'){
					break;
				}
			}
			printError();
			continue;
		}

		int i = 0;
		while (i<len && isWhitespace(pinput[i])){
			i++;
		}

		if (i == len){
			continue;
		}

		if (batchFlag){
			myPrint(pinput);
		}

		i = 0;
		while (i<len && (pinput[i] == '>' || isWhitespace(pinput[i]))){
			i++;
		}

		if (i == len){
			printError();
			continue;
		}

		phrase* currPhrase = parsePhrase(pinput);
		executeAll(currPhrase);

		//Free memory
		while (currPhrase != NULL) {
			while (currPhrase -> currWord != NULL) {
				while (currPhrase -> currWord -> currRTok != NULL) {
					redirectTok* tempRTok = currPhrase -> currWord -> currRTok;
					currPhrase -> currWord -> currRTok = tempRTok -> next;
					free(tempRTok -> currTok);
					free(tempRTok);
				}
				word* tempWord = currPhrase -> currWord;
				currPhrase -> currWord = tempWord -> next;
				free(tempWord);
			}
			phrase* tempPhrase = currPhrase;
			currPhrase = currPhrase -> next;
			free(tempPhrase);
		}
	}

	if (sourceFile != NULL && sourceFile != stdin){
		fclose(sourceFile);
	}
}

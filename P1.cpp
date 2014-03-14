#include<cstdio>
#include<cstdlib>
#include<unistd.h>
#include<pthread.h>
#define NUM_STUDENTS 25

int ticketQueue[250];
int front;
int rear;
int curr_qid;
int curr_sid;
bool workDone = false;
pthread_mutex_t rearMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t checkQueueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t questionMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t answerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t currMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t questionStartDone = PTHREAD_COND_INITIALIZER;
pthread_cond_t answerDone = PTHREAD_COND_INITIALIZER;

bool isTicketQueueEmpty() {
	pthread_mutex_lock(&checkQueueMutex);
	if(front == rear) {
		pthread_mutex_unlock(&checkQueueMutex);
		return true;
	}
	else {
		pthread_mutex_unlock(&checkQueueMutex);
		return false;
	}
}

void AnswerStart() {
	pthread_mutex_lock(&questionMutex);
	pthread_cond_wait(&questionStartDone, &questionMutex);
	pthread_mutex_unlock(&questionMutex);
	int toSleep = rand() % 14 + 2;
	usleep(toSleep * 1000);
}

void AnswerDone() {
	pthread_mutex_lock(&answerMutex);
	printf("Professor is done answering question #%d of student #%d\n", curr_qid, curr_sid);
	pthread_cond_signal(&answerDone);
	pthread_mutex_unlock(&answerMutex);
}

void *TheProfessor(void *nothing) {
	while(!workDone) {
		if(!isTicketQueueEmpty()) {
			printf("Professor is ready to be asked a question\n");
			++front;
			
			AnswerStart();
			printf("Professor is answering question #%d of student #%d\n", curr_qid, curr_sid);
			AnswerDone();
		}
	}
}

void QuestionStart(long sid, int i) {
	printf("Student #%ld is ready to ask question #%d\n", sid, i);
	
	pthread_mutex_lock(&rearMutex);
	int myTicketNum = ++rear;
	pthread_mutex_unlock(&rearMutex);
	
	while(myTicketNum != front) {}
	
	pthread_mutex_lock(&currMutex);
	curr_sid = (int) sid;
	curr_qid = i;
	pthread_mutex_unlock(&currMutex);
	usleep(1000);
}

void QuestionDone(long sid, int i) {
	pthread_mutex_lock(&answerMutex);
	//printf("\nhehe\n");
	pthread_cond_wait(&answerDone, &answerMutex);
	//printf("\nhehe\n");
	pthread_mutex_unlock(&answerMutex);
	printf("Student #%ld has got the answer for question #%d\n", sid, i);
}

void *OneStudent(void *studentid) {
	int numQuestions = rand() % 10 + 1;
	long sid = (long)studentid;
	//printf("Student %ld will ask %d questions\n", sid, numQuestions);

	for(int i = 0; i < numQuestions; ++i) {
		QuestionStart(sid, i);

		pthread_mutex_lock(&questionMutex);
		printf("Student #%ld is asking question #%d\n", sid, i);
		pthread_cond_signal(&questionStartDone);
		pthread_mutex_unlock(&questionMutex);
		
		QuestionDone(sid, i);
	}
}

int main(int argc, char **argv) {
	pthread_t professor;
	pthread_t student[NUM_STUDENTS];
	pthread_attr_t attr;
	void *status;
	int rc;
	
	front = rear = -1;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	rc = pthread_create(&professor, NULL, TheProfessor, (void *)NULL);
	if(rc) {
		printf("Could not create professor thread. Error code: %d\n", rc);
		exit(-1);
	}

	for(long t = 0; t < NUM_STUDENTS; ++t) {
		rc = pthread_create(&student[t], &attr, OneStudent, (void *)t);
		if(rc) {
			printf("Could not create thread for student %ld. Error code: %d\n", t, rc);
			exit(-1);
		}
	}

	for(long t = 0; t < NUM_STUDENTS; t++) {
		rc = pthread_join(student[t], &status);
		if(rc) {
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
        }
    }

    workDone = true;
    pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&rearMutex);
	pthread_mutex_destroy(&checkQueueMutex);
	pthread_mutex_destroy(&questionMutex);
	pthread_mutex_destroy(&answerMutex);
	pthread_mutex_destroy(&currMutex);
	pthread_cond_destroy(&questionStartDone);
	pthread_cond_destroy(&answerDone);
	pthread_exit(NULL);
    return 0;
}

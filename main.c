#include <stdio.h>
#include <pthread.h>
#define MAX 3 //Количетсво передач
pthread_mutex_t the_mutex;
pthread_cond_t condc, condp; //Используется для сигнализации
int buffer = 0; //Буферная переменная между производитель-потребитель

void *producer(void *ptr){
    for(int i = 1; i <= MAX; i++){
        pthread_mutex_lock(&the_mutex); //Получение исключительного доступа к буферу

        while(buffer != 0){
            pthread_cond_wait(&condp,&the_mutex);
            printf("Пробуждение производителя\n");
        }

        buffer = i; //Запись
        printf("Производитель №%d\n",i); //Вывод
        pthread_cond_signal(&condc); //Активизация потребителя
        pthread_mutex_unlock(&the_mutex); //Освобождение доступа к буферу
    }
    pthread_exit(0); //Завершение текущего потока
}

void *consumer(void *ptr){
    for(int i = 1; i <= MAX; i++){
        pthread_mutex_lock(&the_mutex); //Получение исключительного доступа к буферу

        while(buffer == 0){
            pthread_cond_wait(&condc,&the_mutex);
            printf("Пробуждение потребителя\n");
        }

        buffer = 0; //Извличение
        printf("Потребитель №%d\n", i);
        pthread_cond_signal(&condp); //Активизация производителя
        pthread_mutex_unlock(&the_mutex); //Освобождение доступа к буферу
    }
    pthread_exit(0); //Завершение текущего потока
}

int main(int argc, char **argv){
    pthread_t pro, con;
    pthread_mutex_init(&the_mutex,0); //Создаём мьютекс
    pthread_cond_init(&condc,0); //Создание условной переменной condc
    pthread_cond_init(&condp,0); //Создание условной переменной condp
    pthread_create(&con,0,consumer,0); //Создание потока, который будет исполнять функцию consumer(Потребитель)
    pthread_create(&pro,0,producer,0); //Создание потока, который будет исполнять функцию producer(Производитель)
    pthread_join(pro,0); //Ждём завершения потока pro
    pthread_join(con,0); //Ждём завершения потока con
    pthread_cond_destroy(&condc); //Удаление условной переменной condc
    pthread_cond_destroy(&condp); //Удаление условной переменной condp
    pthread_mutex_destroy(&the_mutex); //Удаление мьютекса
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // POSIX API для системных вызовов
#include <signal.h>  // Библиотека для работы с сигналами
#include <arpa/inet.h> // Библиотека для работы с интернет-адресами
#include <sys/socket.h> // Библиотека для операций с сокетами
#include <sys/types.h>  // Библиотека для типов данных
#include <errno.h>   // Библиотека для обработки ошибок

#define PORT 65000 // Определяем порт сервера

volatile sig_atomic_t g_got_sighup = 0; // Глобальная переменная-флаг для обработки сигнала SIGHUP
int g_accepted_socket = -1; // Глобальный дескриптор принятого соединения

// Обработчик сигнала SIGHUP
void handle_signal(int in_signum) {
    if (in_signum == SIGHUP) { // Если пришел сигнал SIGHUP
        g_got_sighup = 1; // Устанавливаем флаг для обработки
    }
}

// Функция для безопасного закрытия серверного сокета
void close_server_socket(int server_socket) {
    if (server_socket != -1) { // Проверяем, что сокет существует
        close(server_socket); // Закрываем сокет
        printf("Серверный сокет закрыт\n"); // Сообщаем о закрытии
    }
}

int main(int argc, char **argv) {
    struct sigaction signal_action; // Структура для настройки обработчика сигнала
    memset(&signal_action, 0, sizeof(signal_action)); // Обнуляем структуру
    signal_action.sa_handler = handle_signal; // Указываем функцию-обработчик
    signal_action.sa_flags = SA_RESTART; // Устанавливаем флаг для перезапуска системных вызовов

    // Устанавливаем обработчик сигнала SIGHUP
    if (sigaction(SIGHUP, &signal_action, NULL) == -1) { // Если установка обработчика завершилась ошибкой
        perror("Ошибка установки обработчика сигнала"); // Сообщаем об ошибке
        exit(EXIT_FAILURE); // Завершаем выполнение программы
    }

    int server_socket; // Сокет сервера
    struct sockaddr_in server_addr; // Структура для хранения адреса сервера
    socklen_t client_len = sizeof(struct sockaddr_in); // Размер структуры адреса клиента

    // Создаем серверный сокет
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) { // Если создание сокета завершилось ошибкой
        perror("Ошибка создания сокета"); // Сообщаем об ошибке
        exit(EXIT_FAILURE); // Завершаем выполнение программы
    }

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET; // Используем IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Разрешаем подключение с любого IP-адреса
    server_addr.sin_port = htons(PORT); // Преобразуем порт в сетевой порядок байтов

    // Привязываем сокет к адресу сервера
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) { // Если привязка сокета завершилась ошибкой
        perror("Ошибка привязки сокета"); // Сообщаем об ошибке
        close_server_socket(server_socket); // Закрываем сокет
        exit(EXIT_FAILURE); // Завершаем выполнение программы
    }

    // Начинаем прослушивать входящие соединения
    if (listen(server_socket, 1) == -1) { // Если настройка прослушивания завершилась ошибкой
        perror("Ошибка при попытке слушать порт"); // Сообщаем об ошибке
        close_server_socket(server_socket); // Закрываем сокет
        exit(EXIT_FAILURE); // Завершаем выполнение программы
    }

    printf("Сервер запущен и слушает порт %d\n", PORT); // Сообщаем, что сервер запущен

    fd_set read_fds; // Набор файловых дескрипторов для pselect
    sigset_t blocked_signals, original_mask; // Наборы сигналов

    // Блокируем сигнал SIGHUP
    sigemptyset(&blocked_signals); // Очищаем набор сигналов
    sigaddset(&blocked_signals, SIGHUP); // Добавляем SIGHUP в набор блокируемых сигналов
    if (sigprocmask(SIG_BLOCK, &blocked_signals, &original_mask) == -1) { // Если блокировка завершилась ошибкой
        perror("Ошибка блокировки сигналов"); // Сообщаем об ошибке
        close_server_socket(server_socket); // Закрываем сокет
        exit(EXIT_FAILURE); // Завершаем выполнение программы
    }

    while (1) { // Бесконечный цикл для работы сервера
        // Проверяем флаг на сигнал SIGHUP
        if (g_got_sighup) { // Если сигнал SIGHUP был получен
            printf("Получен сигнал SIGHUP. Завершаем работу сервера\n"); // Сообщаем о завершении работы
            g_got_sighup = 0; // Сбрасываем флаг
            close_server_socket(server_socket); // Закрываем серверный сокет
            if (g_accepted_socket != -1) // Если есть принятый клиентский сокет
                close(g_accepted_socket); // Закрываем его
            sigprocmask(SIG_SETMASK, &original_mask, NULL); // Сбрасываем маску
            exit(0); // Завершаем выполнение программы
        }

        // Инициализируем набор файловых дескрипторов
        FD_ZERO(&read_fds); // Очищаем набор
        FD_SET(server_socket, &read_fds); // Добавляем серверный сокет
        if (g_accepted_socket != -1) // Если есть клиентский сокет
            FD_SET(g_accepted_socket, &read_fds); // Добавляем его

        // Определяем максимальный дескриптор
        int max_fd = (g_accepted_socket > server_socket) ? g_accepted_socket : server_socket; // Находим максимальный из дескрипторов

        // Ожидаем событий на сокетах с помощью pselect
        int result = pselect(max_fd + 1, &read_fds, NULL, NULL, NULL, &original_mask); // Ожидаем событий
        if (result == -1) { // Если произошла ошибка
            if (errno == EINTR) { // Если ошибка вызвана прерыванием вызова сигналом
                printf("\'pselect\' был прерван сигналом\n"); // Сообщаем об этом
                continue; // Переходим к следующей итерации цикла
            } else {
                perror("Ошибка в pselect");
                break;
            }
        }

        // Проверяем новые запросы на соединение
        if (FD_ISSET(server_socket, &read_fds)) { // Если серверный сокет готов к чтению
            int client_socket = accept(server_socket, NULL, &client_len); // Принимаем новое соединение
            if (client_socket == -1) { // Если произошла ошибка
                perror("Ошибка принятия соединения"); // Сообщаем об ошибке
                continue; // Переходим к следующей итерации
            }
            printf("Принято новое соединение\n"); // Сообщаем о новом соединении

            if (g_accepted_socket == -1) // Если клиентский сокет ещё не установлен
                g_accepted_socket = client_socket; // Сохраняем клиентский сокет
            else { // Если уже есть клиентский сокет
                close(client_socket); // Закрываем лишний сокет
                printf("Закрыто лишнее соединение\n"); // Сообщаем о закрытии
            }
        }

        // Проверяем наличие данных от клиента
        if (g_accepted_socket != -1 && FD_ISSET(g_accepted_socket, &read_fds)) { // Если клиентский сокет готов к чтению
            char buffer[1024]; // Создаем буфер для данных
            ssize_t bytes_received = recv(g_accepted_socket, buffer, sizeof(buffer) - 1, 0); // Читаем данные из сокета
            if (bytes_received > 0) { // Если данные были получены
                buffer[bytes_received] = '\0'; // Завершаем строку нулевым символом
                printf("Получено %ld байт: %s\n", bytes_received, buffer); // Сообщаем о полученных данных
            } else if (bytes_received == 0) { // Если клиент закрыл соединение
                printf("Клиент закрыл соединение\n"); // Сообщаем об этом
                close(g_accepted_socket); // Закрываем клиентский сокет
                g_accepted_socket = -1; // Сбрасываем дескриптор
            } else // Если произошла ошибка при чтении
                perror("Ошибка получения данных"); // Сообщаем об ошибке
        }
    }

    close_server_socket(server_socket); // Закрытие серверного сокета
    return 0; // Завершаем выполнение программы
}

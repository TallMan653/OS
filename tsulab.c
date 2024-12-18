#include <linux/kernel.h> // Основной заголовок ядра Linux
#include <linux/module.h> // Заголовок для создания модулей ядра
#include <linux/init.h> // Заголовок для функций инициализации и завершения модуля
#include <linux/printk.h> // Заголовок для вывода сообщений в лог ядра
#include <linux/proc_fs.h> // Заголовок для работы с файловой системой /proc
#include <linux/uaccess.h> // Заголовок для передачи данных между пространством ядра и пользовательским пространством
#include <linux/version.h> // Заголовок для проверки версии ядра

#define PROCFS_NAME "tsulab" // Имя файла в файловой системе /proc
#define BUFFER_SIZE 256             // Максимальный размер буфера для сообщения
#define START_VALUE 5               // Начальное значение прогрессии
#define RATIO 4                     // Коэффициент прогрессии
#define TERMS 10                    // Количество элементов прогрессии

MODULE_DESCRIPTION("Tomsk State University Kernel Module"); // Описание модуля
MODULE_AUTHOR("Tall Man"); // Информация об авторе

static struct proc_dir_entry *proc_file = NULL; // Указатель на созданный файл в /proc

// Чтение файла в /proc
static ssize_t proc_read(struct file *file, char __user *buffer, size_t len, loff_t *offset)
{
    static char message[BUFFER_SIZE]; // Буфер для сообщения
    static int message_len = 0;       // Длина сообщения

    // Формируем сообщение только при первом чтении
    if (*offset == 0) { 
        int value = START_VALUE;      // Начальное значение прогрессии
        int pos = 0;

        // Генерация последовательности геометрической прогрессии
        for (int i = 0; i < TERMS; i++) {
            pos += scnprintf(message + pos, BUFFER_SIZE - pos, "%d\n", value); // Запись очередного элемента прогрессии
            value *= RATIO; // Умножаем на коэффициент

            if (pos >= BUFFER_SIZE) { // Проверка на переполнение буфера
                pr_warn("Message truncated: buffer size exceeded\n"); // Лог предупреждения о переполнении
                break; // Прекращаем запись, если буфер переполнен
            }
        }
        message_len = pos; // Запоминаем длину созданного сообщения
    }

    // Если смещение больше или равно длине сообщения, данные прочитаны полностью
    if (*offset >= message_len) {
        return 0; // Конец чтения
    }

    // Ограничиваем длину доступного для чтения фрагмента
    if (len > message_len - *offset) {
        len = message_len - *offset; 
    }

    // Копирование данных из буфера ядра в пространство пользователя
    if (copy_to_user(buffer, message + *offset, len)) {
        pr_err("Failed to copy data to user space\n"); // Лог ошибки при неудачном копировании
        return -EFAULT; // Возврат ошибки
    }

    *offset += len; // Обновляем смещение (Все же решил добавлять)
    return len; // Возвращаем количество переданных данных
}

// Проверка версии ядра для выбора структуры операций файла
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read, // Указатель на функцию чтения для современных ядер
};
#else
static const struct file_operations proc_file_ops = {
    .read = proc_read, // Указатель на функцию чтения для старых ядер
};
#endif

// Инициализация модуля
static int __init tsu_init(void)
{
    pr_info("Welcome to the Tomsk State University\n"); // Лог сообщения при загрузке модуля

    // Создание файла в /proc с указанными правами
    proc_file = proc_create(PROCFS_NAME, 0644, NULL, &proc_file_ops); 
    if (!proc_file) { // Проверка успешности создания файла
        pr_err("Failed to create /proc/%s\n", PROCFS_NAME); // Лог ошибки при неудачном создании файла
        return -ENOMEM; // Возврат ошибки памяти
    }

    pr_info("/proc/%s created\n", PROCFS_NAME); // Лог успешного создания файла
    return 0; // Успешное завершение функции инициализации
}

// Завершение работы модуля
static void __exit tsu_exit(void)
{
    proc_remove(proc_file); // Удаление файла из /proc
    pr_info("/proc/%s removed\n", PROCFS_NAME); // Лог успешного удаления файла
    pr_info("Tomsk State University forever!\n"); // Лог сообщения при выгрузке модуля
}

module_init(tsu_init); // Указание функции инициализации модуля
module_exit(tsu_exit); // Указание функции завершения работы модуля
MODULE_LICENSE("GPL"); // Указание лицензии модуля

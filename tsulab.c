#include <linux/kernel.h> // Основной заголовок ядра Linux, предоставляет базовые функции ядра
#include <linux/module.h> // Заголовок для создания модулей ядра
#include <linux/init.h> // Заголовок для функций инициализации и завершения модуля
#include <linux/printk.h> // Заголовок для вывода сообщений в лог ядра через функции pr_info, pr_err и т.д.
#include <linux/proc_fs.h> // Заголовок для работы с файловой системой /proc
#include <linux/uaccess.h> // Заголовок для передачи данных между пространством ядра и пользовательским пространством
#include <linux/version.h> // Заголовок для проверки версии ядра, используется для совместимости кода с разными версиями

MODULE_DESCRIPTION("Tomsk State University Kernel Module"); // Описание модуля
MODULE_AUTHOR("Tall Man"); // Автор модуля

#define PROCFS_NAME "tsulab" // Имя файла в файловой системе /proc

static struct proc_dir_entry *proc_file = NULL; // Указатель на структуру файла в /proc
static unsigned long current_value = 5; // Начальное значение прогрессии
static const unsigned long multiplier = 4; // Множитель для прогрессии

// Функция чтения из /proc/tsulab
static ssize_t proc_read(struct file *file, char __user *buffer, size_t len, loff_t *offset)
{
    char new_line[32]; // Буфер для новой строки прогрессии
    size_t new_line_len; // Длина новой строки

    // Если смещение больше нуля, значит данные уже были прочитаны
    if (*offset > 0)
        return 0;

    // Формирование строки с текущим значением прогрессии
    new_line_len = snprintf(new_line, sizeof(new_line), "%lu\n", current_value);
    current_value *= multiplier; // Увеличиваем значение прогрессии

    // Копируем строку в пользовательское пространство
    if (copy_to_user(buffer, new_line, new_line_len))
        return -EFAULT; // Возвращаем ошибку, если копирование не удалось

    *offset += new_line_len; // Обновляем смещение
    return new_line_len; // Возвращаем длину записанных данных
}

// Определение структуры proc_ops или file_operations в зависимости от версии ядра
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read, // Устанавливаем функцию чтения
};
#else
static const struct file_operations proc_file_ops = {
    .read = proc_read, // Устанавливаем функцию чтения
};
#endif

// Функция инициализации модуля
static int __init tsu_init(void)
{
    // Создаем файл в файловой системе /proc
    proc_file = proc_create(PROCFS_NAME, 0644, NULL, &proc_file_ops);
    if (!proc_file) {
        pr_err("Failed to create /proc/%s\n", PROCFS_NAME); // Логируем ошибку создания файла
        return -ENOMEM; // Возвращаем код ошибки памяти
    }

    pr_info("/proc/%s created\n", PROCFS_NAME); // Логируем успешное создание файла
    return 0; // Возвращаем 0 при успешной инициализации
}

// Функция завершения работы модуля
static void __exit tsu_exit(void)
{
    proc_remove(proc_file); // Удаляем файл из файловой системы /proc
    pr_info("/proc/%s removed\n", PROCFS_NAME); // Логируем удаление файла
}

// Устанавливаем функции инициализации и завершения работы модуля
module_init(tsu_init);
module_exit(tsu_exit);

MODULE_LICENSE("GPL"); // Устанавливаем лицензию GPL для модуля

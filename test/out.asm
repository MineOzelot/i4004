%include "vm.inc"

    jun entry
str:
    db "Hello, World!\n"
entry:
    fim r0:r1, str  ; Загружаем адрес строки в пару r0:r1
    fim r2:r3, 2    ; Загружаем число (16 - размер строки) в r3 для isz
print:
    fin r4:r5       ; Читаем байт по адресу в r0:r1 и записываем в r4:r5
    fim r8:r9, 0xC0 ; Загружаем адрес первой половинки в интерфейсе vm
    src r8:r9       ; Отправляем адрес
    ld  r5          ; Загружаем значение из r5 в аккумулятор (2-я половинка байта символа)
    wrm             ; Записываем из аккумулятора в память
    fim r8:r9, 0xC1 ; Загружаем адрес второй половинки в интерфейсе vm
    src r8:r9
    ld  r4          ; Загружаем значение из r4 в аккумулятор (1-я половинка байта символа)
    wrm
    wr0             ; Обновляем 0-ю половинку состояния в интерфейса vm, т.о. вызывая вывод символа
    inc r1          ; Инкрементируем указатель на строку
    isz r3, print   ; Инкрементируем r3; Если r3 не ноль, то возвращаемся на print

    vm_terminate
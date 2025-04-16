This text Russian language.
English version issue #18

Данные проект поддерживает pgAdmin3 v1.22
Поддержка добавляется по мере возникновения ошибок в оригинальной версии v1.22 или если эти возможности нужны мне.
На 10.10.2018 измененены около 70 исходных файлов.

Для удобства последний скомпилированный исполняемый файл будет находиться в каталоге Release.
Для работы достаточно заменить оригинальный pgAdmin3.exe.

Будет поддерживаться только оригинальная версия PostgreSQL 12 и PostrgesPro Enterprise.

Полная версия pgAdmin3 находиться тут https://github.com/postgres/pgadmin3.git

Что добавлено:
 - Экспорт результата запроса в Excel
 - Добавлен выбор запроса на исполнение под курсором (Auto-Select)
 - Добавлена настраиваемая автозамена (в меню Правка -> Manage autoreplace)
 - Добавлено автосохранение содержимого закладки после выполнения запроса
 - Добавлена возможность задать имя для закладки и возможность сделать закладку автозагружаемой для конкретной БД

 - Добавлена поддержка процедур
 - Добавлена поддержка секционирования (только отображение в дереве объектов)

 - Удалено отображение узлов имеющих статус (Never execute) на закладке графического плана, но в табличном виде они присутствуют.

01.11.2018
 - Добавлено отображение publications.
 - Добавлено изменение фона при при не закоммиченой транзакции.
 - У Commit/Rollback измененены горячие клавиши

11.12.2018
 - Добавлен поиск в дереве по F4 выделеного текста и если объект найден то его открытие.
   Если запрос длится более 2 минут то после завершения запроса окно будет мигать.
 - При открытия функции фокус устанавливается сразу на закладку Код.

05.12.2018
 - Добавлена поддержка расширения pgpro_scheduler
   В разделе Статистика отображается информация о последнем отработавшем задании.
   Инфомация берётся из лог таблицы pg_log при условии что таблица существует и видна, установлен флаг "Enabled ASUTP style"
   выводиться результат запроса: select log_time,detail critical,message,application_name from pg_log l where l.log_time>'$Started'::timestamp - interval '1min' and l.log_time<'$Finised' and hint='$name'
 - В выводе результатов запроса ячейки со значениям содержащие символ перевода строки \n подсвечиваются
 * В экспорте результатов запроса в Excel исправлена ошибка при сохранении интервалов
 * При обновлении схемы не блокируется интерфейс если на таблице идет долгая операция cluster
   Но при F5 на самой таблице блокировка сохраняется (это связано с блокированием функций pg_def* при получинии информации от таблицах)
09.12.2018
 - autocomplite: добавлены имена функций, и возможность подставлять имена колонок таблиц из поля FROM
 - при наборе имени функции появляется перечень параметров этой функции

28.12.2018
 - выполнен переход на wxWidgets 3.0 версия exe файла будет находиться Release_(3.0)
 - в текстовом представлении плана можно сворачивать узлы
 - построении плана с замерами в заголовках строк указывается процент времени выполнения узла (только операции узла, но не вложенных узлов)


11.01.2019
 - исправлены падения приложения при открытии таблицы по нажатию F4

26.01.2019
 - исправлены падения приложения при вводе ( в окне редактирования кода)
 - ускорено открытие диалога "новая функция", "новая таблицы".

09.02.2019
 - исправлены некторые ошибки
 - добавлено копирование sql в html формате(с сохранением цвета)
 - в вывод SQL инструкций для таблиц добавлен закомментированый перечень колонок с типами

11.03.2019
 - исправлено отображение foreign table

10.09.2019 
 Окно Server Status

 * исправлено падение окна Server Status при аварийном завершении СУБД
 - добавлена расцветка процессов которые блокируют другие процессы

 Окно Query

 - добавлен фильтр в окно результатов запроса. Активируется двойным щелчком мыши по ячейке, текст которой и будет являтся условием фильтра. Снимается из контекстного меню.
   При нажатом Alt условие отбора инвертируется (Скрыть строки содержащие значение).
 - Для избегания ожиданий при получении информации об объектах. Выставляется клиентский параметр SET lock_timeout=15000 для служебного соединения.

04.09.2019
 - добавлена поддержка PostgreSQL 12
 - добавлена поддержка отображения дополнительных опция для индексов
 - в окне запросов добавлена альтернативная кнопка отражающая текущий режим, Transaction (T) или AutoCommit (A)
 * исправлена ошибка в окне поиска объектов при поиске в коментариях

22.12.2019
 - добавлена возможность выполнять сравнение описания объектов разных серверов через меню Отчеты "Compare other objects"
   Сравнение проводится с другим открытым соединением и подключенной базой. Объекты для сравнения выбираются по дереву вниз.
   По результатам формируется html отчет различий. 
   В качестве шаблона для отчета используется файл textcompare_report.template, находящийся рядом с исполняемым pgadmin3.exe.
   Особенности: SQL текст создания последовательностей игнорируется, секции таблиц не учитываются. Полность одинаковые объекты скрываются. Служебные объекты игнорируются.
 - выполнен переход на новые библиотеки dll wxWidgets 3.0.4 скомпилированные под VS2012. Необходимо обновить файлы *.dll
 
04.03.2020
 - добавлен вывод CREATE STATISTICS для таблиц
 * исправлен вывод SQL команды для создания задания для комманд заданных в виде массива

28.03.2020
 - добавлена информация о фрагментации таблицы (cfs_fragmentation)
 * убрано предупреждение о версии сервера

11.04.2020
 - добавлена многоколоночная сортировка результатов выполнения запроса. Порядок сортировки колонок и направление отмечается цветными индикаторами (RED,YELLOW,GREEN,BLUE,GREY).
   Максимальное число колонок сортировки 5. Для выполнения сортировки нужно щелкнуть по заголовку колонки удерживая клавишу Alt.
 - добавлены новые опции для Vaccum ( DISABLE_PAGE_SKIPPING ) и Reindex ( CONCURRENTLY )
 * ускорена работа фильтра в окне результав запроса.

13.04.2020
 * исправлено падение в режиме редактирования
 * исправлено редактирование процедур без аргументов

15.04.2020
 * в окне SQL инструкции создания таблицы теперь отображаются новые параметры хранения
 * в описании колонок учтены generated и identity колонки

22.04.2020
 - добавлена возможность создавать дополнительные окна для вывода результатов запроса (не более 9).
   Для этого запрос нужно выполнить по нажатию Shift+F8.  Вывод результатов при выполнении F8 производиться в текущую активную закладку.
   Окна вывода отмечаются белым квадратом если они были использованы текущей закладкой запросов.
 - при щелчке правой кнопкой мыши на активной закладке результатов в окне запросов выделяется запрос связанный с этим результатом.
 - в окне запросов последний выполненый запрос отмечается зелеными стрелками.
 - при автосохранении закладок, сохраняется позиция курсора

06.05.2020
 * исправлена проблема #4 (Crash after close sql editor)
  
08.05.2020
 * исправлена проблема #6 (Child tables are not dispayed). Отображение секций из других схем нарушает строгую иерархичность обектов 
   и нужно убедиться что всё нормально в вашем случае. Секции всегда группируются в узел Partitions который находиться в родительской таблице.
   В родной схеме, секции как таблицы увидеть нельзя.
 * мелкие улучшения

02.09.2020
 - добавлена возможность копировать в буфер обмена выделенные ячейки результата запроса в формате IN списка и Where конструкций. Вызывается из контекстного меню.
 - в Server status окне добавлена возможность фильтровать строки по щелчку правой кнопкой мыши.
 * иправлено issues #8 (dropping overloaded procedures)
 
05.09.2020 
 - при сравнении объектов добавлена возможность исключать сравнение привелегий и комментариев.
 * исправлено копирование текста запроса из под фильтра в Server status окне. При сравнении текста из колонки Client порт не учитывается.

05.12.2020
 * много мелких исправлений
 * добавлены некоторые новые возможности PG13. Описание в коммитах.
 - добавлена проверка btree индекса. Проверка выполняется функцией bt_index_parent_check(regclass,true) из расширения amcheck. 
   Расширение должно быть установлено в БД к которой происходит подключение pgadmin3.

02.01.2021
 - добавлена сортировка на вкладках "свойства" , "Статистика" и других.
 - по секционированным таблицам в статистеке выводятся все потомки.
 
03.01.2021
 - скомпилирована 64 битная версия pgAdmin3.exe
 - 32 битная больше не поддерживается

19.02.2021
 -  Сохраняется расположении окон при скрытии outputPane и применяется при его показе.
 -  Появилась возможность менять иконку query окна.
    Есть два способа изменения icon для окна query.
    1.  Поместить новую icon в %APPDATA%\postgresql\icons
        Имя файла задать следующим образом: hostname_dbname.png или hostname.png или dbname.png
        Размер icon 32х32
    2.  Задать для сервера цвет. Фон icon будет окрашен в цвет сервера.

19.08.2021
 - добавлено окно просмотра CSV лога базы.
   Окно вызывается из контекстного меню сервера "Log view ...".
   
   После открытия окна читается непосредственно файл лога функцией pg_read_binary_file
   Выбирается файл с самой свежей датой изменения. Проверка новых сообщений проводиться каждые 5 секунд.
   Можно добавить другие сервера на панели "Settings". Настройки применяются после закрытия окна и повторного его открытия.
   Если окно лога не активное и приходит сообщения уровня Error и выше, то иконка отмечается красным квадратом.
   Если на заладке "Settings" выбрано несколько серверов, то происходить автоматическое подключение к ним.
   После подключения все открытые сервера в дереве объектов можно закрыть одной командой контекстного меню 
   "Disconnect all servers".
   ВНИМАНИЕ: память требуемая для хранения логов ни чем не ограничивается (кроме фильтрации на этапе загрузки лога) и 
   возможно выделения большого количества памяти.
   Отображаются строки лога в двух режимах:
   * Простой. Отобразаются все полученные строки лога
   * Групповой. Строки с похожими сообщениями объединяются в группу и видимой строкой является самая последняя строка
     в группе. Для просмотра всех строк группы нужно установить флаг "View detail group".
     Сообщения будут похожими если они отличаются только числами и если они не в двойных кавычках.
     В групповом режиме в поле host показываются счетчик свежих сообщений попавших в группу. Счетчик сбрасывается при 
     установке курсора на строку группы.
   Для исключения из просмотра ненужных строк используются поколоночные фильтры. Для включения фильтра нужно:
   * Щелкнуть правой кнопкой мыши по полю. Для инверсии фильтра нужно удерживать Ctrl.
   * Выбрать значение в контекстном меню заголовка колонки. Там отображаются 20 самых частых значения в колонке с указанием
     количества этих значений.
   * Ввести в поле значения для фильтра, выделить это значение и нажать Enter. Для фильтра используется только выделенный 
     текст. Такой фильтр будет работать на поиск выделенного вхождения в поле. Если в выделенной строке 
     первым символом будет "!" то фильтер инверсируется.
   * каждое отдельное значение фильтра можно удалить через контекстное меню заголовка колонки.
     Для более высокой производительности рекомендуется проводить загрузку логов с включенным "Mode group".
     Или сбрасывать "Mode group", но при установленных фильтрах.
     Отображение большого число строк (более 10000 ) происходит несколько секунд и более.
   * Есть возможность отсеять строки на этапе загрузки. Для этого установите фильтры на строки и нажмите
     "Add Filter Ignore" этот фильтр будет записан в файл filter_load.txt.

13.09.2021
 - Добавлено меню закрытия всех открытых серверов "Disconnect all servers"
13.01.2022
 - Для Log view добавлена: подержка быстрой навигации: Shift+KeyUP,KeyDOWN
   переход на запись с тем же sql_state,
   Alt+KeyUP,KeyDOWN - переход на запись с другим sql_state
   Добавлена колонка Server - сервер с которого получен лог.
   * Ctrl+S отправка сообщения по почте Outlook. Шаблон письма в файле mail.template
     В первых двух строках шаблона можно указать адреса которые будут подставляться в письмо.
 - В frmLog добалены сохраняемые пользовательские фильтры.
   По кнопке Add текущий фильтр сохраняется. Имя задается в ComboBox.
 * В окне "Status Server" устанавливается парамер "SET statement_timeout=10000;" и "SET log_min_messages = FATAL"
   чтобы избежать зависания функции pg_query_state.
 * При возникновении ошибки "server closed the connection unexpectedly"
   сообщение об этом теперь не выводиться на экран. Т.к. происходило падение pgAdmin3

06.07.2022
Добавлена частичная поддержка возможностей PG15:
- поддержка списка колонок при задании FK
- поддержка NULLS NOT DISTINCT для уникальных индексов

24.11.2022
  Добавлена эксперементальная функция работы с gitLab.
  Для работы с gitlab необходимо положить файл gitlab.json в каталог %APPDATA%\postgresql.
  Вот пример содержимого файла:
{
"url": "https://gl.mympany.ru:4443/api/v4/",
"private_token": "V3JYpw2x5rr61yGe_M2e",
"project_id": "532"
}
 После запуска pgAdmin3 появится вкладка Git на которой будут дополнительные закладки.
 Пока для работы с GitLab можно выполнять только коммиты на дополнительной вкладке Commit.
 В GitLab сохраняются только содержимое объектов схем. Сохраняется только SQL представления.
Алгоритм работы такой:
 - из файл gitlab.json берётся информация для соединения с gitLab
 - из ветки по умолчанию (обычно это main) считывается файл pgadmin3.json с общими настройки.
 - если такого файла не то настройки беруться из gitlab.json
 - вот пример настроек из pgadmin3.json
{
"ignore_schema": ["public","repack","schedule"],
"control_objects": ["Functions","Views","Tables","Trigger Functions","Procedures","Schemas","Schema","Database"],
"maps_branch_to_dbname":[
	{"branch": "asu",
         "list_db": ["asu"]
        }
        ,
       	{"branch": "common_db",
         "list_db": ["dbname1","dbname2"]
        }

]
}
Где:
 "ignore_schema"   - список схем которые не нужно сохранять и git
 "control_objects" - перечень типов объектов схемы которые нужно сохранять.
 "maps_branch_to_dbname" - сопостовление имен веток и имен БД.
 - нажимается кнопка "Load Git" и загружается SQL предствления объектов из GitLab
   После этой операции список "List commit files" будет заполнен расхождениями текущей БД и веткой в GitLab
 - Если выбрать несколько элементов (или все нажав Ctrl+A) то указав название коммита и нажав на
   кнопку "commit" можно закомитить текущее SQL представление выбранных элементов в GitLab
 - нажатие правой кнопки мыши на каком либо элементе списка покажет различия между объектов в БД и GitLab.
Все прочие кнопки и закладки использовать не нужно.
Типовой способ использования, ведение историй изменений объектов БД в GitLab.

02.05.2023
  Добавлена возможность выравнивания списков команд insert и других структурированных данных (списки IN).
  В настройках можно задать внешнюю утилиту которая на вход принимает выделенный текст а на выходе выдаёт выровненый.
  Если утилиту не задавать то выравнивание будет выполнено pgadmin3 (код проверенен не полностью, возможны зависания)
  Подробное описание в commit https://github.com/levinsv/pgadmin3/commit/c197ea45c18385204497a1f53f1fda184c6cc86b

22.05.2023
  Для улучшения наглядности и понимания в какой БД мы находимся в строке
  браузера объектов при выделении элемента будет напротив отображаться имя
  БД. Это поведение можно отключить в настройках.
 
06.06.2023
  Если поиск в дереве объектов начинается с сервера, то он продолжается только среди серверов без поиска в глубину.


29.06.2023

  pgAdmin3.exe собран с новой версий wxWidgets 3.2 для улучшения работы с DPI.
  Обновите wx\*.dll файлы.
  Исполняемый файл компилируется со манифестом для указания поддержки DPI.
  Появилась возможность заменить PNG иконки в toolbar на svg иконки.
  Для этого нужно создать каталог svg в каталоге исполняемого файла и поместить файлы файлы с раширением svg.
  Имя файла должно быть таким же как и имя файла оригинального PNG из каталога include/images.
  На данныый момент можно заменить иконки основного окна, окна запроса, и окна статуса сервера.
  Имена файлов можно найти в исходных кодах поиском строки \*GetBundleSvg.
  Во время первого запуска возможно авариное завершение pgAdmin3.exe.
  Перед первым запуском сохраните копию файла autoSaveConfig.reg
  Если будет такая потребность можно и другие иконки перевести на svg.

  Выполнена оптимизация по производительности обновления дерева объектов и отображение результатов запросов.


02.08.2023

  * исправлено зацикливание программы при выполнении pgScript.(изменились правила обработки wxRegEx)
  * устранены утечки GDI объектов.
  - добавлена возможность скрытия/отображения панели с историей запросов.
  - добавлены некторые возможности PG16:
        * отображение новых опций члена роли SET и INHERIT. В диалогах их установить нельзя.


05.08.2023

  * Можно включить более понятное отображение больших чисел на странице Статистика.
    Для этого устновите флажок "Beautiful big numbers on the statistics page".

27.10.2023

  * Можно задать дополнительные параметры соединения https://github.com/levinsv/pgadmin3/commit/0093e3676c480cd6886a66feb10cb26d99a2e315
  * Добавлена возможность отключения/включения автосохранения запросов и быстрых переходов к корневым узлам дерева объектов
    подробности в commit https://github.com/levinsv/pgadmin3/commit/bce303c437944ab4ad13bcc7303dbe644a92618a
  * Генерация AWR отчета если установлено расширение pgpro_pwr https://github.com/levinsv/pgadmin3/commit/c139994efa9bdafd235e3d620fe4ed05946f7330

02.02.2024

  - При помощи контекстной команды "Compare 2 Cells" можно сравнить 2 не пустые ячейки в результе запроса.
  - При потере фокуса окна редактора запроса,
    вернуть его можно было только нажав на окно левой кнопкой мыши.
    Теперь это можно сделать нажав на имя закладки.
  * Исправлена проблема при автовыборе запроса содержащего многострочный комментарий с символом ;
  * Исправлена проблема c отображением свойства is_cycled последовательности.
    Начиная с 10 версии это свойство ошибочно всегда определялось как false.

24.09.2024

  - Компиляция для win с использованием настройки stdcpp17
  - В редактор запросов добавлен диалог трансформации текста при помощи PCRE выражения, с подсветкой синтаксиса и найденных групп.
    Вызывается Ctrl+M. Настройки диалога сохраняются в pgadmin3opt.json.
    Статья в wiki https://github.com/levinsv/pgadmin3/wiki/Transformation-text
  - Для редактирования pgadmin3opt.json добавлен элемент в диалог настройки. Json настройки представлены в виде дерева.
    Insert - добавляет/копирует элемент в массив. Delete - Удаляет. Ctrl+Z - отменить изменения.(но не удаление/вставку)
    Ctrl+F - поиск строки.
  - В окне "Status Server" при загрузке логов появилась навигационная панель с цветовыми индикаторами. Настройка и доступные команды в json файле.
    Добавлены несколько цветных индикаторов для примера. Справка по командам по F1.
    Работает только с CSV логами.
  - В окне "Status Server" добавлена возможность включить сбор ожиданий процессов.
    Для этого должно быть установлено расширение pg_wait_sampling.
    Ожидание ClientRead собирается только тогда открыта транзакция и для неё получен идентификатор.
    Т.е. это ожидание данных от клиента только при начатых транзакциях.
    Статья в wiki https://github.com/levinsv/pgadmin3/wiki/Waits-events

17.12.2024

 - Добавлена возможность быстрой подстановки слов на латинице по нажатию
   Alt+RIGHT. Возможность включается настройкой "Use word hints".
   Список слов составляется при загрузке запроса и по мере ввода новых слов.
 - При выполнении запроса добавлена возможность заменять переменные вида
   $1, $2, ... или :variableName на пользовательские значения введённые
   в диалоге. Пока можно заменять переменные в запросах select,update,delete,insert.
   Перед отправкой запроса на сервер переменные заменяются простой текстовой заменой.
   Запрос который выполнен на сервере можно посмотреть на вкладке История.
   Возможность включается настройкой "Replace variables in a query".
   Выделить правой кнопкой выполненый запрос не получиться т.к. текст выполненого запроса
   и текст в редакторе будет отличаться.
   Значения для замены сохраняются в pgadmin3opt.json (при завершении программы).

10.02.2025

 - Расширены возможности автоподстановки.
   Добавлена подстановка соединений таблиц(и представлений) по их FK.
 - Подстановка работает в двух вариантах:
       * После ключевого слова ON:самая правая таблица соединяется с любой левой.
       * После ключевого слова WHERE AND OR все таблицы соединяются со всеми.
   Дополнение условия соединения после символа = .
   Представления можно соединить только если поле представления является полем таблицы.
   Подстановка работает нормально если для таблиц используются синонимы.
  * Стандартное автодополнение теперь выдаёт список таблиц и представление после JOIN.

25.03.2025

 - Добавлен ключ -el для экспорта информации о серверах в файл настроей linux версии pgadmin3.
 - Исправления для linux версии:
       * В окне "Status Server" снижено мерцание при обновлении строк активных процессов
         Была добавлена фиктивная строка в конец списка процессов при использовании фильтра.
       * В ctlSQLGrid добавлена проверка на совпадение цветов сетки и заголовков строк.
 - Изменения в графическом отображении плана запроса:
    * Добавлено изображения двух узлов плана Partial GroupAggregate, Finalize GroupAggregate.
    * Узел Memoize рисуется динамически с процентной полоской попаданий в кеш(Hits), идикацией Evictions, Overflows.
    * Добавлена поддержка колеса мыши.
    * Если в плане более 300 узлов, то включается оптимизация отрисовки (возможно появление артефактов при прокрутке экрана).



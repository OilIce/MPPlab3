Сборка:

Для этой работы использовлась библиотека SFML 3.0.2. Для её подключаения в Visual Studio требуется:
1. Скачать библиотеку: https://www.sfml-dev.org/download/sfml/3.0.2/ и распаковать архив;
2. В Properties проекта подключить папку SFML/include в C++/General.Additional Include;
3. В Properties проекта подключить папку SFML/lib в Linker/General/Additional Libraries;
4. В Properties проекта включить файлы sfml-window-d.lib;sfml-audio-d.lib;sfml-graphics-d.lib;sfml-network-d.lib;sfml-system-d.lib в Linker/Input/Additional Dependencies;
5. Дополнительые библиотеки уже включены в проект (все файлы, начинающиеся с sfml-).

Для смены режима (CCD/MR) нажмите SPACE. 

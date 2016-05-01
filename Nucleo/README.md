# GSM_FibocomG510
Przykładowy projekt pokazujący jak używać rozszerzenie GSM od MikroTar.pl z Nucleo. 

Jak użyć ten kod w System Workbench:

1. Stwórz nowy projekt w System Workbench dla Twojej płytki
2. Usuń z niego plik main.c (wybierz main.c, menu podręczne, 'delete')
3. Zaznacz folder 'src'; z menu podręcznego wybierz 'new>folder'
4. W otwartym okienku dialogowym kliknij 'advanced' i zaznacz 'link to alternate location'; wciśnijcie klawisz 'browse' i wybierz katalog w którym jest plik 'main.c'
5. Wyczyść projekt 


Czasami występują problem z plikiem startup/startup_stm32f411xe.s. Jest tak, gdy w projekcie folder /startup ułożony jest przed /src. Jedyne rozwiązanie, jakie znalazłem, to przesunąć plik 'startup/startup_stm32f411xe.s' do folderu '/src'. 


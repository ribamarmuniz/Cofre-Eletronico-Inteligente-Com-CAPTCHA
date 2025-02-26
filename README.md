# Cofre Eletrônico Inteligente com CAPTCHA Físico - Sistemas Embarcados

Este repositório apresenta o desenvolvimento de um Cofre Eletrônico Inteligente baseado em sistemas embarcados, utilizando Raspberry Pi Pico W e protocolos de comunicação I2C, ADC e GPIOs. O projeto implementa um mecanismo de autenticação em duas etapas, combinando senha e um CAPTCHA físico interativo.

✨ Funcionalidades

🔑 Autenticação por Senha

O usuário insere uma senha de 4 dígitos via teclado matricial 4x4.

O sistema verifica a senha e permite até 3 tentativas incorretas.

Após 3 tentativas erradas, o acesso é bloqueado por 10 segundos com aviso no LCD 16x2 e na matriz de LEDs 5x7.

Durante o bloqueio:

Um buzzer apita a cada segundo.

LEDs vermelhos piscam em movimento "vai e vem" acompanhando a contagem regressiva.

Após o tempo, o cofre retorna ao estado inicial.

🎮 Verificação por CAPTCHA Físico

Caso a senha esteja correta, o sistema não concede acesso imediato.

É gerado um desafio interativo utilizando uma matriz de LEDs WS2812b 5x7.

O usuário deve mover um LED azul (ponto inicial) até um LED vermelho (ponto final) utilizando um joystick analógico pelo menor caminho possível.

Se movimentos inconsistentes forem detectados, o acesso é negado automaticamente.

O usuário tem 20 segundos para concluir a verificação.

Se bem-sucedido, o cofre é aberto por 10 segundos antes de fechar automaticamente.

🗓 Registro de Acessos e Tentativas

Todas as tentativas de acesso são registradas com data e hora utilizando um RTC DS3231.

Caso ocorra uma tentativa inválida:

O LCD exibe um aviso.

O buzzer é ativado.

LEDs RGB acendem na cor vermelha.

Se a verificação for bem-sucedida:

A matriz de LEDs acende na cor azul.

LEDs RGB indicam sucesso na cor verde.

O buzzer emite um beep curto.

Os LEDs RGB e o buzzer dão feedback visual e sonoro:

Verde: Sucesso na autenticação.

Vermelho: Falha ou bloqueio.

Pulsação de LEDs acompanhando a contagem regressiva.

Buzzer sonoro indicando sucesso ou erro.

⏰ Fechamento Automático

Após ser desbloqueado, o cofre permanece aberto por 10 segundos.

O sistema exibe uma contagem regressiva no LCD e na matriz de LEDs.

Ao final do tempo, o cofre se fecha automaticamente.

🌐 Estrutura do Projeto

Diagrama de Blocos

O sistema é composto por quatro camadas:

Interface do Usuário: Teclado matricial, LCD, Joystick.

Lógica de Controle: Processamento dos comandos e decisões.

Comunicação: Interação entre dispositivos.

Hardware: Sensores e atuadores controlados pelo firmware.


![image](https://github.com/user-attachments/assets/067401de-9e86-4bb9-a8cc-bde9f48bd813)

O fluxograma abaixo ilustra o funcionamento do cofre:


![image](https://github.com/user-attachments/assets/a7bcf5e9-f01b-43f2-b0c0-4581fa3b2654)

🔧 Componentes Utilizados

Microcontrolador: Raspberry Pi Pico W

Autenticação: Teclado Matricial 4x4

Segurança Adicional: Joystick Analógico + Matriz de LEDs WS2812b 5x7

Feedback Visual: LCD 16x2 + LEDs RGB

Feedback Sonoro: Buzzer

Registro de Eventos: RTC DS3231

Alimentação: Fonte de energia regulada

🔄 Fluxo de Operação

Usuário insere a senha no teclado matricial.

Sistema verifica a senha:

Senha errada: Mensagem de erro, decremento de tentativas.

Três erros: Cofre bloqueado por 10 segundos.

Se a senha estiver correta, inícia-se a verificação CAPTCHA.

Usuário move o LED azul até o vermelho com o joystick:

Sucesso: Cofre é aberto por 10 segundos.

Falha: Acesso negado.

Cofre se fecha automaticamente após o tempo determinado.

💡 Conclusão

O Cofre Eletrônico Inteligente demonstrou-se eficiente para segurança e controle de acesso, incorporando autenticação dupla e registro de eventos. O CAPTCHA físico agregou uma camada extra de segurança, impedindo acessos automatizados.

Os testes confirmaram que o sistema registra tentativas, bloqueia após falhas e oferece uma interação fluida entre hardware e software.

Este projeto pode ser aplicado em diversas situações onde é necessária segurança reforçada de baixo custo.

By: José Ribamar Cerqueira
Link da simulação: https://wokwi.com/projects/423282301499206657

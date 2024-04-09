#include <IRremote2.h>
#include <AccelStepper.h>

//Setup do controle InfraRed
const int RECV_PIN = 49;
IRrecv irrecv(RECV_PIN);
decode_results results;

//Pinagem dos motores
//Esquerda Cima
AccelStepper stepper1(AccelStepper::HALF4WIRE,17,15,16,14);
//Esquerda Baixo
AccelStepper stepper2(AccelStepper::HALF4WIRE,57,55,56,54);
//Direita Cima (inverso)
AccelStepper stepper3(AccelStepper::HALF4WIRE,8,10,9,11);
//Direita Baixo
AccelStepper stepper4(AccelStepper::HALF4WIRE,29,25,27,23);

//Setup dos Sensores

#define SensorEsquerdaBaixo digitalRead(47)
#define SensorEsquerdaCima digitalRead(50) 
#define SensorCimaEsquerda digitalRead(51) 
#define SensorCimaDireita digitalRead(37)
#define SensorDireitaCima digitalRead(39) 
#define SensorDireitaBaixo digitalRead(41) 
#define SensorBaixoDireita digitalRead(45) 
#define SensorBaixoEsquerda digitalRead(43) 

//Variáveis
bool Cima = false;
bool Baixo = false;
bool Esquerda = false;
bool Direita = false;
bool InicioMovimento = false;
bool AjusteGeral = false;
bool MovimentoTeste = true;
bool Torto = false;
int velocidade = 600;
int count = 0;
int correcao = 0;
int tempoDelay = 0;
int ultimoTorto = -1;

int ultimoEsquerdaBaixo = 0;
int ultimoEsquerdaCima = 0;
int ultimoCimaEsquerda = 0;
int ultimoCimaDireita = 0;
int ultimoDireitaCima = 0;
int ultimoDireitaBaixo = 0;
int ultimoBaixoDireita = 0;
int ultimoBaixoEsquerda = 0;

//Variáveis ajustáveis
int sensibilidadeSensor = 100;
int TimerTimeout = 0;
int TempoMaximo = 15000;

//Variável para teste
int ultimoTeste = 0;

void setup() {
  //Setup da serial
  Serial.begin(9600);
  Serial.println("Start..");

  //Setup dos sensores
  pinMode(37, INPUT);
  pinMode(39, INPUT);
  pinMode(41, INPUT);
  pinMode(43, INPUT);
  pinMode(45, INPUT);
  pinMode(47, INPUT);
  pinMode(50, INPUT);
  pinMode(51, INPUT);

  //Setup de um motor, o único necessário
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(17, OUTPUT);

  //Velocidade máxima para os motores
  stepper1.setMaxSpeed(1000);
  stepper2.setMaxSpeed(1000);
  stepper3.setMaxSpeed(1000);
  stepper4.setMaxSpeed(1000);

  //Setup do sensor InfraRed
  irrecv.enableIRIn();
}

void limparVariaveisMovimento() {
  InicioMovimento = true;
  Esquerda = false;
  Cima = false;
  Baixo = false;
  Direita = false;
  TimerTimeout = millis();
}

void loop() {
  //Setup para receber os comandos via InfraRed
  if (irrecv.decode(&results)) { 
    Serial.println(results.value, HEX);
    
    //Receber próximo comando
    irrecv.resume();

    //Se o robô está parado
    if(!Esquerda && !Cima && !Direita && !Baixo && !AjusteGeral) {
      switch(results.value) {
        case 0xFF22DD:
        case 0xFF0008F7:
        //Esquerda
        limparVariaveisMovimento();
        Esquerda = true;
        break;
        
        case 0xFF629D:
        case 0xFF0019E6:
        //Cima
        limparVariaveisMovimento();
        Cima = true;
        break;
        
        case 0xFFC23D:
        case 0xFF005EA1:
        //Direita
        limparVariaveisMovimento();
        Direita = true;
        break;
        
        case 0xFFA857:
        case 0xFF0016E9:
        //Baixo
        limparVariaveisMovimento();
        Baixo = true;
        break;
      }
    }
  }

// ==== TESTES NOVO ====

//if(!SensorDireitaBaixo) {
//  InicioMovimento = true;
//  Esquerda = false;
//  Cima = false;
//  Baixo = true;
//  Direita = false;
//}
//
//
//if(!SensorDireitaCima) {
//  InicioMovimento = true;
//  Esquerda = false;
//  Cima = true;
//  Baixo = false;
//  Direita = false;
//}

// ==== SETUP TIMERS ====

  setupTimers();

// ==== MOVIMENTOS COM DELAY ====

  if(Cima) {
    // Se andou mais que o {TempoMaximo}, faz o robô parar
    if(timeout()) {
      limparVariaveisMovimento();
    } else {
      if(InicioMovimento && !valorSensorBaixoEsquerda() && !valorSensorBaixoDireita() && !valorSensorCimaEsquerda() && !valorSensorCimaDireita()) {
        InicioMovimento = false;
      }
      // Se chegou ao final
      if(!InicioMovimento && valorSensorBaixoEsquerda() && valorSensorBaixoDireita() && valorSensorCimaEsquerda() && valorSensorCimaDireita()) {
        // Anda um pouco a mais para tentar deixar o sensor no meio da fita
        for (int i=0; i<10000; i++) {
          andarCima();
          runSteppers();
        }
        // Arruma as variáveis para acabar com o movimento, e começar os ajustes
        Cima = false;
        AjusteGeral = true;
      }
      else if(verificarIndoCima()) {
        andarCima();
      }
    }
  }

  if(Baixo) {
    if(timeout()) {
      limparVariaveisMovimento();
    } else {
      if(InicioMovimento && !valorSensorBaixoEsquerda() && !valorSensorBaixoDireita() && !valorSensorCimaEsquerda() && !valorSensorCimaDireita()) {
        InicioMovimento = false;
      }
      if(!InicioMovimento && valorSensorBaixoEsquerda() && valorSensorBaixoDireita() && valorSensorCimaEsquerda() && valorSensorCimaDireita()) {
        for (int i=0; i<10000; i++) {
          andarBaixo();
          runSteppers();
        }
        Baixo = false;
        AjusteGeral = true;
      }
      else if(verificarIndoBaixo()) {
        andarBaixo();
      }
    }
  }

  if(Esquerda) {
    if(timeout()) {
      limparVariaveisMovimento();
    } else {
      if(InicioMovimento && !valorSensorEsquerdaCima() && !valorSensorEsquerdaBaixo() && !valorSensorDireitaCima() && !valorSensorDireitaBaixo()) {
        InicioMovimento = false;
      }
      if(!InicioMovimento && (valorSensorEsquerdaCima() && valorSensorEsquerdaBaixo() && valorSensorDireitaCima() && valorSensorDireitaBaixo())) {
        for (int i=0; i<10000; i++) {
          andarEsquerda();
          runSteppers();
        }
        Esquerda = false;
        AjusteGeral = true;
      }
      else if(verificarIndoEsquerda()) {
        andarEsquerda();
      }
    }
  }

  if(Direita) {
    if(timeout()) {
      limparVariaveisMovimento();
    } else {
      if(InicioMovimento && !valorSensorDireitaCima() && !valorSensorDireitaBaixo() && !valorSensorEsquerdaCima() && !valorSensorEsquerdaBaixo()) {
        InicioMovimento = false;
      }
      if(!InicioMovimento && (valorSensorDireitaCima() && valorSensorDireitaBaixo() && valorSensorEsquerdaCima() && valorSensorEsquerdaBaixo())) {
        for (int i=0; i<10000; i++) {
          andarDireita();
          runSteppers();
        }
        Direita = false;
        AjusteGeral = true;
      }
      else if(verificarIndoDireita()) {
        andarDireita();
      }
    }
  }


  if(AjusteGeral) {
    switch(ultimoTorto){
      case 0:
        while(!valorSensorEsquerdaBaixo() || !valorSensorEsquerdaCima() || !valorSensorDireitaCima() || !valorSensorDireitaBaixo()) {
          setupTimers();
          andarEsquerda();
          runSteppers();
        }
        for (int i=0; i<10000; i++) {
          andarEsquerda();
          runSteppers();
        }
      break;
      case 1:
        while(!valorSensorCimaEsquerda() || !valorSensorCimaDireita() || !valorSensorBaixoEsquerda() || !valorSensorBaixoDireita()) {
          setupTimers();
          andarCima();
          runSteppers();
        }
        for (int i=0; i<10000; i++) {
          andarCima();
          runSteppers();
        }
      break;
      case 2:
        while(!valorSensorEsquerdaBaixo() || !valorSensorEsquerdaCima() || !valorSensorDireitaCima() || !valorSensorDireitaBaixo()) {
          setupTimers();
          andarDireita();
          runSteppers();
        }
        for (int i=0; i<10000; i++) {
          andarDireita();
          runSteppers();
        }
      break;
      case 3:
        while(!valorSensorCimaEsquerda() || !valorSensorCimaDireita() || !valorSensorBaixoEsquerda() || !valorSensorBaixoDireita()) {
          setupTimers();
          andarBaixo();
          runSteppers();
        }
        for (int i=0; i<10000; i++) {
          andarBaixo();
          runSteppers();
        }
      break;
    }

    //Finaliza o movimento reiniciando as variáveis
    ultimoTorto = -1;
    Baixo = false;
    Cima = false;
    Direita = false;
    Esquerda = false;
    InicioMovimento = true;
    AjusteGeral = false;
  }


  //Ativa os motores caso esteja andando normalmente
  if((Cima || Baixo || Esquerda || Direita) && !AjusteGeral) {
    runSteppers();
  }
}

// ====== FUNÇÕES ======

boolean verificarIndoEsquerda() {
//Correção de quando encosta na linha no final do movimento para esquerda, para ajustar o ângulo
  if(valorSensorEsquerdaCima() && !valorSensorEsquerdaBaixo() && !InicioMovimento) {
    //Se for a primeira vez que está corrigindo (ultimoTorto == -1)
    //E checar se ele está tão torto que os sensores de CIMA na ESQUERDA ou de BAIXO na ESQUERDA estão fora, significa que ele precisa ir para cima
    if(ultimoTorto == -1 && (!valorSensorCimaEsquerda() || !valorSensorBaixoEsquerda())) {
      ultimoTorto = 1;
    }
    girarDireita();
    //Retorna false para ele usar o girarDireita() chamado aqui, no runSteppers do loop principal, sem usar o andarEsquerda() (da função de if(Esquerda))
    return false;
  }
  else if(!valorSensorEsquerdaCima() && valorSensorEsquerdaBaixo() && !InicioMovimento) {
    if(ultimoTorto == -1 && (!valorSensorCimaEsquerda() || !valorSensorBaixoEsquerda())) {
      ultimoTorto = 3;
    }
    girarEsquerda();
    return false;
  }

  return true;
}

boolean verificarIndoDireita() {
  if(valorSensorDireitaCima() && !valorSensorDireitaBaixo() && !InicioMovimento) {
    if(ultimoTorto == -1 && (!valorSensorCimaDireita() || !valorSensorBaixoDireita())) {
      ultimoTorto = 1;
    }
    girarEsquerda();
    return false;
  }
  else if(!valorSensorDireitaCima() && valorSensorDireitaBaixo() && !InicioMovimento) {
    if(ultimoTorto == -1 && (!valorSensorCimaDireita() || !valorSensorBaixoDireita())) {
      ultimoTorto = 3;
    }
    girarDireita();
    return false;
  }

  return true;
}

boolean verificarIndoCima() {
  if(valorSensorCimaEsquerda() && !valorSensorCimaDireita() && !InicioMovimento) {
    if(ultimoTorto == -1 && (!valorSensorEsquerdaCima() || !valorSensorDireitaCima())) {
      ultimoTorto = 0;
    }
    girarEsquerda();
    return false;
  }
  else if(!valorSensorCimaEsquerda() && valorSensorCimaDireita() && !InicioMovimento) {
    if(ultimoTorto == -1 && (!valorSensorEsquerdaCima() || ! valorSensorDireitaCima())) {
      ultimoTorto = 2;
    }
    girarDireita();
    return false;
  }

  return true;
}

boolean verificarIndoBaixo() {
  if(valorSensorBaixoEsquerda() && !valorSensorBaixoDireita() && !InicioMovimento) {
    if(ultimoTorto == -1 && (!valorSensorEsquerdaBaixo() || ! valorSensorDireitaBaixo())) {
      ultimoTorto = 0;
    }
    girarDireita();
    return false;
  }
  else if(!valorSensorBaixoEsquerda() && valorSensorBaixoDireita() && !InicioMovimento) {
    if(ultimoTorto == -1 && (!valorSensorEsquerdaBaixo() || ! valorSensorDireitaBaixo())) {
      ultimoTorto = 2;
    }
    girarEsquerda();
    return false;
  }

  return true;
}

// ==== SETUP VARIÁVEIS DE DELAY ====

// nome da variável -> "valor" + {NomeDoSensor}
boolean valorSensorEsquerdaBaixo() {
  //Retorno true se ele estiver contando o tempo depois de ter detectado algo (ultimoEsquerdaBaixo != 0)
  //E se já tiver passado o tempo (sensibilidadeSensor), de quando começou a detectar algo
  //*Então quanto maior o valor na variável "sensibilidadeSensor", mais tempo vai demorar para o sistema entender que o sensor detectou algo
  return ultimoEsquerdaBaixo != 0 && ultimoEsquerdaBaixo + sensibilidadeSensor < millis();
}

boolean valorSensorEsquerdaCima() {
  return ultimoEsquerdaCima != 0 && ultimoEsquerdaCima + sensibilidadeSensor < millis();
}

boolean valorSensorCimaEsquerda() {
  return ultimoCimaEsquerda != 0 && ultimoCimaEsquerda + sensibilidadeSensor < millis();
}

boolean valorSensorCimaDireita() {
  return ultimoCimaDireita != 0 && ultimoCimaDireita + sensibilidadeSensor < millis();
}

boolean valorSensorDireitaCima() {
  return ultimoDireitaCima != 0 && ultimoDireitaCima + sensibilidadeSensor < millis();
}

boolean valorSensorDireitaBaixo() {
  return ultimoDireitaBaixo != 0 && ultimoDireitaBaixo + sensibilidadeSensor < millis();
}

boolean valorSensorBaixoDireita() {
  return ultimoBaixoDireita != 0 && ultimoBaixoDireita + sensibilidadeSensor < millis();
}

boolean valorSensorBaixoEsquerda() {
  return ultimoBaixoEsquerda != 0 && ultimoBaixoEsquerda + sensibilidadeSensor < millis();
}

//Função para manipulação da varíavel de "ultima detectação" ou "de quando começou a detectar algo"
void setupTimers() {
  //Se o sensor detectou algo e se não é o primeiro tick de deteção (se ele já não está contando o tempo de detecção)
  if(SensorEsquerdaBaixo && ultimoEsquerdaBaixo == 0) {
    //Seta o início de detecção para o tempo atual
    ultimoEsquerdaBaixo = millis();
    //Caso o sensor tenha detectado vazio (branco), ele reinicia a variável
  } else if(!SensorEsquerdaBaixo && ultimoEsquerdaBaixo != 0) {
    ultimoEsquerdaBaixo = 0;
  }

  if(SensorEsquerdaCima && ultimoEsquerdaCima == 0) {
    ultimoEsquerdaCima = millis();
  } else if(!SensorEsquerdaCima && ultimoEsquerdaCima != 0) {
    ultimoEsquerdaCima = 0;
  }

  if(SensorCimaEsquerda && ultimoCimaEsquerda == 0) {
    ultimoCimaEsquerda = millis();
  } else if(!SensorCimaEsquerda && ultimoCimaEsquerda != 0) {
    ultimoCimaEsquerda = 0;
  }

  if(SensorCimaDireita && ultimoCimaDireita == 0) {
    ultimoCimaDireita = millis();
  } else if(!SensorCimaDireita && ultimoCimaDireita != 0) {
    ultimoCimaDireita = 0;
  }

    if(SensorDireitaCima && ultimoDireitaCima == 0) {
    ultimoDireitaCima = millis();
  } else if(!SensorDireitaCima && ultimoDireitaCima != 0) {
    ultimoDireitaCima = 0;
  }

  if(SensorDireitaBaixo && ultimoDireitaBaixo == 0) {
    ultimoDireitaBaixo = millis();
  } else if(!SensorDireitaBaixo && ultimoDireitaBaixo != 0) {
    ultimoDireitaBaixo = 0;
  }

  if(SensorBaixoDireita && ultimoBaixoDireita == 0) {
    ultimoBaixoDireita = millis();
  } else if(!SensorBaixoDireita && ultimoBaixoDireita != 0) {
    ultimoBaixoDireita = 0;
  }

  if(SensorBaixoEsquerda && ultimoBaixoEsquerda == 0) {
    ultimoBaixoEsquerda = millis();
  } else if(!SensorBaixoEsquerda && ultimoBaixoEsquerda != 0) {
    ultimoBaixoEsquerda = 0;
  }
}

boolean timeout() {
  return (TimerTimeout + TempoMaximo) < millis();
}


// ==== FUNÇÕES BÁSICAS ====

void andarBaixo() {
  stepper1.setSpeed(velocidade);
  stepper2.setSpeed(velocidade);
  stepper3.setSpeed(velocidade);
  stepper4.setSpeed(velocidade);
}

void andarCima() {
  stepper1.setSpeed(-velocidade);
  stepper2.setSpeed(-velocidade);
  stepper3.setSpeed(-velocidade);
  stepper4.setSpeed(-velocidade);
}

void andarDireita() {
  stepper1.setSpeed(-velocidade);
  stepper2.setSpeed(velocidade);
  stepper3.setSpeed(velocidade);
  stepper4.setSpeed(-velocidade);
}

void andarEsquerda() {
  stepper1.setSpeed(velocidade);
  stepper2.setSpeed(-velocidade);
  stepper3.setSpeed(-velocidade);
  stepper4.setSpeed(velocidade);
}

void girarDireita() {
  stepper1.setSpeed(-velocidade);
  stepper2.setSpeed(-velocidade);
  stepper3.setSpeed(velocidade);
  stepper4.setSpeed(velocidade);
}

void girarEsquerda() {
  stepper1.setSpeed(velocidade);
  stepper2.setSpeed(velocidade);
  stepper3.setSpeed(-velocidade);
  stepper4.setSpeed(-velocidade);
}

void runSteppers() {
  stepper1.run();
  stepper2.run();
  stepper3.run();
  stepper4.run();
}

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


//Variáveis
bool Cima = false;
bool Baixo = false;
bool Esquerda = false;
bool Direita = false;
bool Stop = false;
bool InicioMovimento = false;
bool AjusteGeral = false;
bool MovimentoTeste = true;
bool Torto = false;
int velocidade = 600;
int count = 0;
int correcao = 0;
int tempoDelay = 0;

//Variável para teste
int ultimoTeste = 0;

String incoming = "";

void setup() {
  //Setup da serial
  Serial.begin(9600);
  Serial.println("Start..");
  Serial1.begin(115200);

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
  Stop = false;
  Serial.println("Limpado");
}

void loop() {
  if (Serial1.available() > 0) {
      incoming  = Serial1.readString();
      Serial.println(incoming);
      incoming.trim();

      if(incoming == "UP") {
        goCima();
      }
      if(incoming == "DOWN") {
        goBaixo();
      }
      if(incoming == "LEFT") {
        goEsquerda();
      }
      if(incoming == "RIGHT") {
        goDireita();
      }
      if(incoming == "STOP") {
        goStop();
      }
  }

  //Setup para receber os comandos via InfraRed
  if (irrecv.decode(&results)) { 
    Serial.println(results.value, HEX);
    
    //Receber próximo comando
    irrecv.resume();

    //Se o robô está parado
    switch(results.value) {
      case 0xFF22DD:
      case 0xFF0008F7:
      //Esquerda
      goEsquerda();
      break;
      
      case 0xFF629D:
      case 0xFF0019E6:
      //Cima
      goCima();
      break;
      
      case 0xFFC23D:
      case 0xFF005EA1:
      //Direita
      goDireita();
      break;
      
      case 0xFFA857:
      case 0xFF0016E9:
      //Baixo
      goBaixo();
      break;
      
      case 0xFF02FD:
      case 0xBF4000FF:
      goStop();
      break;
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

// ==== MOVIMENTOS COM DELAY ====

  if(Cima) {
    andarCima();
  }

  if(Baixo) {
    andarBaixo();
  }

  if(Esquerda) {
    girarEsquerda();
  }

  if(Direita) {
    girarDireita();
  }

  if(Stop) {
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

// ==== COMANDOS ====

void goCima() {
  limparVariaveisMovimento();
  Cima = true;
}

void goBaixo() {
  limparVariaveisMovimento();
  Baixo = true;
}

void goEsquerda() {
  limparVariaveisMovimento();
  Esquerda = true;
}

void goDireita() {
  limparVariaveisMovimento();
  Direita = true;
}

void goStop() {
  limparVariaveisMovimento();
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

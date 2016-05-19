# Projeto 2: Desempenho do Processador

Matheus Yokoyama Figueiredo     137036

Klaus Rollmann                  146810

Carlos Gregoreki                104721

## Introdução

## Metodologia
O desempenho do processador foi comparado seguindo quatro pontos principais, são eles:
* Estrutura do Pipeline
* Forwarding de Dados
* Configurações de Cache
* Branch Predictor

Para isso, o simulador do processador MIPS foi alterado para contabilizar stalls perdidos por hazards de dados e controle para cada configuração do processador. Além disso, também foram contabilizados stalls devido ao acesso à memória, usando o simulador de cache `dinneroiv`. O processador usado como base nos testes foi o de 5 estágios, e foram feitos testes com 11 configurações distintas para poder avaliar cada um dos quatro fatores que influenciam no desempenho do processador. 

#### Estrutura do Pipeline

O pipeline de 5 estágios foi comparado com processadores com 7 e 13 estágios, e também com um processador superescalar de 5 estágios. Assim foi possível verificar tanto a influência do aumento no número de estágios, quanto o efeito de um processador superescalar no desempenho.

<<< processador com múltiplos estágios >>>
 
 No processador superescalar, em cada estágio do pipeline são executadas duas instruções ao mesmo tempo. Por um lado, o processador executa duas instruções por ciclo de clock, o que melhora o número de instruções por segundo, mas o número de hazards também aumenta, e mais stalls são necessários.
 
#### Hazards e Forwarding de Dados
 
Os hazards de dados foram contabilizados tanto no processador escalar de 5 estágios como no superescalar. O forwarding de dados foi analisado somente no processador escalar. Os outros processadores foram considerados já com o forwarding de dados.
 
##### Forwarding de dados
Para o processador escalar de 5 estágios, foi contabilizada a influência do forwarding de dados feito na ALU. Isso foi feito fazendo-se três verificações durante a execução do simulador.

1. **Instrução atual lê um registrador escrito pela instrução anterior:** Quando isso ocorre e não há forwarding, são adicionados 2 stalls. Com o forwarding não há hazard e não são necessários stalls.

2. **Instrução atual lê um registrador escrito pela instrução antes da anterior:** Esse caso é similar ao anterior, porém a instrução já está um estágio distante da anterior, então só é necessário 1 stall caso não houver forwarding. Com o forwarding também não há penalidades nesse caso.

3. **Instrução atual lê um registrador que foi escrito por uma instrução de load anterior:** Quando isso ocorre, mesmo com o forwarding, é necessário adicionar um stall, pois a instrução de load só obtém o dado no estágio seguinte em que a instrução atual está.

Essas verificações foram feitas modificando-se o arquivo `mips.isa` para salvar as duas instruções anteriores, e também quais os registradores foram lidos e quais foram escritos por essas instruções

##### Hazard de Dados

Nos processadores escalares com múltiplos estágios e superescalar, foi considerada uma implementação com o forwarding. Para o processador com múltiplos estágios, <<<< processador com multiplos estágios >>>
No processador superescalar, foi verificado o caso em que uma instrução lê um registrador que foi escrito por uma instrução de load anterior, e também, se ocorre dependencia de instruções e essas instruções estão no mesmo estágio do pipeline. Esses hazard são os do tipo WAR e WAW.
O processador superescalar suposto foi um que possui 5 estágios, em que há dois pipelines em paralelo, como mostrado na figura abaixo.

<img src="https://upload.wikimedia.org/wikipedia/commons/thumb/4/46/Superscalarpipeline.svg/978px-Superscalarpipeline.svg.png" width="450">

Como há duas instruções no mesmo estágio do pipeline, pode ocorrer dependência entre duas instruções que escrevem (WAW) ou se uma instrução escreve e outra lê, se estiverem no mesmo estágio do pipeline. Sendo assim, foram feitas duas verificações, descritas abaixo.

1. **Uma das duas instruções atuais lê um registrador que foi escrito por uma instrução de load dentre as duas anteriores:** Essa verifiação é a mesma feita para o processador escalar, mas agora tomando o cuidado para verificar a dependência entre uma das duas instruções atuais com uma das duas instruções anteriores, já que são executadas em paralelo. Nesse caso foi adicionado um stall em um dos pipelines.

2. **As instruções executadas em paralelo dependem uma da outra (WAR ou WAW):** Nesse caso, se duas instruções estão no mesmo estágio do pipeline e tem alguma dependência de WAR ou WAW, então é contabilizado um stall em um dos pipelines.

Para contabilizar essas dependências, foram salvas as duas instruções atuais e as duas intruções anteriores. Como o simulador obtém uma instrução por vez, cada nova instrução obtida em ciclos ímpares é salva como uma das instruções atuais, e nos ciclos pares são feitas as verificações. Ao final, o número de ciclos foi dividido pela metade, já que no pipeline superescalar as duas instruções são executadas em um mesmo ciclo. 

##### Configurações de Cache


##### Branch Predictor
A avaliação dos branch predictors tentou verificar o tipo de influência que os branches causam no número de ciclos, e também analisar duas alternativas usadas para tentar prever a ocorrência de um salto do tipo branch. O *branch always taken* e o *1-bit branch taken* foram os tipos de branch predictor analisados.
Para os saltos não condicionais, do tipo jump, foram contabilizados dois stalls, independentemente do branch predictor utilizado. Nos casos de saltos condicionais, são somados três ciclos caso o branch predictor erre na predição e nenhum ciclo caso ele acerte.
A implementação dessas verificações consistiu em verificar se um branch ocorre ou não e contabilizar o número de ciclos. No branch predictor de *1 bit*, foi preciso também salvar uma variável que indica o estado do último branch e fazer algumas verificações adicionais. Quando ocorre branch e a variável está setada como *false*, então contabiliza os ciclos adicionais e altera o valor da variável. De forma similar, se o branch não ocorre e a variável está setada como *true*, então adiciona os três ciclos adicionais. Nos outros casos o branch predictor acerta e não são contabilizados ciclos adicionais.

##### Cálculo e Medição
As variáveis medidas no simulador foram:

* Número de ciclos
* Número total de instruções
* Número de stalls de Dados
* Número de stalls de Branch
* Número de stalls de Jump
* Quantidade de acertos do Branch predictor
* Simulation speed

No simulador de cache, foram obtidas as seguintes informações:

* Total de miss na cache L1
* Miss rate na cache L1
* Total de miss na cache L2
* Miss rate na cache L2

A partir dessas variáveis foram calculadas as seguintes informações:
* **Total de Ciclos (cache + miss):** O total de ciclos para os processadores escalares foi contabilizado como:

        (ciclos + 10 * miss l1 + 600* miss l2)

    , em que o número de ciclos já inclui os stalls.
    No processador superescalar, o número de ciclos contabilizado no simulador foi dividido pela metade. Porém, se considerarmos que os stalls de branch e jump afetam os dois pipelines, então suas penalidades devem ser consideradas duas vezes. Assim, os stalls de branch e jump foram contabilizados mais uma vez e o número de ciclos para esse processador é calculado como:

        (ciclos + jump stalls + branch stalls + 10*miss l1 + 600* miss l2)
        
* **Tempo:** O tempo de execução foi calculado considerando a velocidade da simulação dada pelo simulador (*instr/sec*) e considerando que um ciclo corresponde ao tempo para executar uma instrução. Essa aproximação supõe que cada estágio leva um ciclo e cada ciclo uma instrução é executada - ou, nos casos em que há stall, uma instrução de NOP é executada. Assim, foi possível contabilizar o tempo baseando-se na velocidade do simulador como se fosse a taxa de *ciclos/sec* do processador. Essa aproximação permite contabilizar um tempo que varia proporcionalmente com o número de ciclos e depende da configuração do processador escolhida.

* **CPI:** O número de ciclos por instrução também foi calculado. Para isso foi feita a divisão do número total de ciclos pelo número de instruções executadas

## Resultados

## Análise

## Conclusão

# Projeto 2: Desempenho do Processador

Matheus Yokoyama Figueiredo     137036

Klaus Rollmann                  146810

Carlos Gregoreki                104721

## Introdução
Processamento em computação nem sempre é melhorado através de freqüência de clock, mas também por adoções arquiteturais adequadas, avaliando o tamanho pipeline e qual branch predictor adotar, além de qual configuração de cache utilizar. Assim, é possível aumentar a eficiência de um processador e até mesmo reduzir seu consumo de energia com algo que não necessariamente signifique aumento da frequência de seu clock. Neste presente projeto, avaliamos o impacto da adoção de algumas configurações de cache, da adoção de branch predictors ou não num processador, bem como a observação do impacto na alteração de tamanho de pipeline.

## Metodologia
O desempenho do processador foi comparado seguindo quatro pontos principais, são eles:
* Estrutura do Pipeline
* Forwarding de Dados
* Configurações de Cache
* Branch Predictor

Para isso, o simulador do processador MIPS foi alterado para contabilizar stalls perdidos por hazards de dados e controle para cada configuração do processador. Além disso, também foram contabilizados stalls devido ao acesso à memória, usando o simulador de cache `dineroiv`. O processador usado como base nos testes foi o de 5 estágios, e foram feitos testes com 11 configurações distintas para poder avaliar cada um dos quatro fatores que influenciam no desempenho do processador. 

#### Estrutura do Pipeline

O pipeline de 5 estágios foi comparado com processadores com 7 e 13 estágios, e também com um processador superescalar de 5 estágios. Assim foi possível verificar tanto a influência do aumento no número de estágios, quanto o efeito de um processador superescalar no desempenho.

Nos processadores com múltiplos estágios, a primeira instrução precisa passar por todos os estágios antes de concluir, e em seguida, cada instrução é completada a cada ciclo. Sendo assim, o número de ciclos muda somente para a primeira instrução. Essa análise é simplificada, e não considera o efeito dos hazards ou mudança no tempo de cada estágio, pois isso depende de como são distribuídos os estágios e também do processador específico. Os testes seguintes desconsideraram o tempo para a primeira instrução completar por ser uma medida muito pequena.
 
No processador superescalar, em cada estágio do pipeline são executadas duas instruções ao mesmo tempo. por um lado, o processador executa duas instruções por ciclo de clock, o que melhora o número de instruções por segundo, mas o número de hazards também aumenta, e mais stalls são necessários.
 
#### Hazards e Forwarding de Dados
 
Os hazards de dados foram contabilizados tanto no processador escalar de 5 estágios como no superescalar. O forwarding de dados foi analisado somente no processador escalar. Os outros processadores foram considerados já com o forwarding de dados.
 
##### Forwarding de dados
Para o processador escalar de 5 estágios, foi contabilizada a influência do forwarding de dados feito na ALU. Isso foi feito fazendo-se três verificações durante a execução do simulador.

1. **Instrução atual lê um registrador escrito pela instrução anterior:** Quando isso ocorre e não há forwarding, são adicionados 2 stalls. Com o forwarding não há hazard e não são necessários stalls.

2. **Instrução atual lê um registrador escrito pela instrução antes da anterior:** Esse caso é similar ao anterior, porém a instrução já está um estágio distante da anterior, então só é necessário 1 stall caso não houver forwarding. Com o forwarding também não há penalidades nesse caso.

3. **Instrução atual lê um registrador que foi escrito por uma instrução de load anterior:** Quando isso ocorre, mesmo com o forwarding, é necessário adicionar um stall, pois a instrução de load só obtém o dado no estágio seguinte em que a instrução atual está.

Essas verificações foram feitas modificando-se o arquivo `mips.isa` para salvar as duas instruções anteriores, e também quais os registradores foram lidos e quais foram escritos por essas instruções.

##### Hazard de Dados

Nos processadores escalares com múltiplos estágios e superescalar, foi considerada uma implementação com o forwarding. 
No processador superescalar, foi verificado o caso em que uma instrução lê um registrador que foi escrito por uma instrução de load anterior, e também, se ocorre dependencia de instruções e essas instruções estão no mesmo estágio do pipeline. Esses hazard são os do tipo WAR e WAW.
O processador superescalar suposto foi um que possui 5 estágios, em que há dois pipelines em paralelo, como mostrado na figura abaixo.

<img src="https://upload.wikimedia.org/wikipedia/commons/thumb/4/46/Superscalarpipeline.svg/978px-Superscalarpipeline.svg.png" width="450">

Como há duas instruções no mesmo estágio do pipeline, pode ocorrer dependência entre duas instruções que escrevem (WAW) ou se uma instrução escreve e outra lê, se estiverem no mesmo estágio do pipeline. Sendo assim, foram feitas duas verificações, descritas abaixo.

1. **Uma das duas instruções atuais lê um registrador que foi escrito por uma instrução de load dentre as duas anteriores:** Essa verifiação é a mesma feita para o processador escalar, mas agora tomando o cuidado para verificar a dependência entre uma das duas instruções atuais com uma das duas instruções anteriores, já que são executadas em paralelo. Nesse caso foi adicionado um stall em um dos pipelines.

2. **As instruções executadas em paralelo dependem uma da outra (WAR ou WAW):** Nesse caso, se duas instruções estão no mesmo estágio do pipeline e tem alguma dependência de WAR ou WAW, então é contabilizado um stall em um dos pipelines.

Para contabilizar essas dependências, foram salvas as duas instruções atuais e as duas intruções anteriores. Como o simulador obtém uma instrução por vez, cada nova instrução obtida em ciclos ímpares é salva como uma das instruções atuais, e nos ciclos pares são feitas as verificações. Ao final, o número de ciclos foi dividido pela metade, já que no pipeline superescalar as duas instruções são executadas em um mesmo ciclo. 

#### Configurações de Cache
As configurações de Cache foram escolhidas para tentar verificar a influência do tamanho do bloco, associatividade e tamanho do cache na quantidade de misses. As configurações escolhidas foram as com melhor desempenho, obtidas através da análise feita no exercício 2 da disciplina.

#### Branch Predictor
A avaliação dos branch predictors tentou verificar o tipo de influência que os branches causam no número de ciclos, e também analisar duas alternativas usadas para tentar prever a ocorrência de um salto do tipo branch. O *branch always taken* e o *1-bit branch taken* foram os tipos de branch predictor analisados.
Para os saltos não condicionais, do tipo jump, foram contabilizados dois stalls, independentemente do branch predictor utilizado. Nos casos de saltos condicionais, são somados três ciclos caso o branch predictor erre na predição e nenhum ciclo caso ele acerte.
A implementação dessas verificações consistiu em verificar se um branch ocorre ou não e contabilizar o número de ciclos. No branch predictor de *1 bit*, foi preciso também salvar uma variável que indica o estado do último branch e fazer algumas verificações adicionais. Quando ocorre branch e a variável está setada como *false*, então contabiliza os ciclos adicionais e altera o valor da variável. De forma similar, se o branch não ocorre e a variável está setada como *true*, então adiciona os três ciclos adicionais. Nos outros casos o branch predictor acerta e não são contabilizados ciclos adicionais.

#### Cálculo e Medição
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

        ciclos + 10 * miss l1 + 600* miss l2

    , em que o número de ciclos já inclui os stalls.
    No processador superescalar, o número de ciclos contabilizado no simulador foi dividido pela metade. Porém, se considerarmos que os stalls de branch e jump afetam os dois pipelines, então suas penalidades devem ser consideradas duas vezes. Assim, os stalls de branch e jump foram contabilizados mais uma vez e o número de ciclos para esse processador é calculado como:

        (ciclos + jump stalls + branch stalls)/2 + 10*miss l1 + 600* miss l2
        
* **Tempo:** O tempo de execução foi calculado considerando a velocidade da simulação dada pelo simulador (*instr/sec*) e considerando que um ciclo corresponde ao tempo para executar uma instrução. Essa aproximação supõe que cada estágio leva um ciclo e em cada ciclo uma instrução é executada - ou, nos casos em que há stall, uma instrução de NOP é executada. Assim, foi possível contabilizar o tempo baseando-se na velocidade do simulador como se fosse a taxa de *ciclos/sec* do processador. Essa aproximação permite contabilizar um tempo que varia proporcionalmente com o número de ciclos e depende da configuração do processador escolhida.

* **CPI:** O número de ciclos por instrução também foi calculado. Para isso foi feita a divisão do número total de ciclos pelo número de instruções executadas

## Resultados

| Config | Programa       | Tempo (s) | Ciclos     | CPI     | Branch correct predictions | Branch correctness ratio | Cache miss rate L1 | Cache miss rate L2 | Stalls de dados e controle |
|--------|----------------|-----------|------------|---------|----------------------------|--------------------------|--------------------|--------------------|----------------------------|
| 1      | Basic Math     |           | 1089764742 | 1,0000  |                            |                          |                    |                    |                            |
|        | Rijndael Coder |           | 43561038   | 1,0000  |                            |                          |                    |                    |                            |
|        | Susan Corners  |           | 3222157    | 1,0000  |                            |                          |                    |                    |                            |
| 2      | Basic Math     |           | 1089764744 | 1,0000  |                            |                          |                    |                    |                            |
|        | Rijndael Coder |           | 43561040   | 1,0000  |                            |                          |                    |                    |                            |
|        | Susan Corners  |           | 3222159    | 1,0000  |                            |                          |                    |                    |                            |
| 3      | Basic Math     |           | 1089764750 | 1,0000  |                            |                          |                    |                    |                            |
|        | Rijndael Coder |           | 43561046   | 1,0000  |                            |                          |                    |                    |                            |
|        | Susan Corners  |           | 3222165    | 1,0000  |                            |                          |                    |                    |                            |
| 4      | Basic Math     | 26,90     | 1208737318 | 1,1092  | 67993852                   | 53,94%                   | 0,30%              | 2,20%              | 1136902918                 |
|        | Rijndael Coder | 14,71     | 781287742  | 17,9355 | 393183                     | 44,18%                   | 4,34%              | 49,00%             | 29745350                   |
|        | Susan Corners  | 0,10      | 4534586    | 1,4073  | 186777                     | 55,37%                   | 0,12%              | 36,94%             | 3295439                    |
| 5      | Basic Math     | 30,43     | 1443687788 | 1,3248  | 67993852                   | 53,94%                   | 0,30%              | 2,20%              | 258519560                  |
|        | Rijndael Coder | 13,43     | 790855640  | 18,1551 | 393183                     | 44,18%                   | 4,34%              | 49,00%             | 2660056                    |
|        | Susan Corners  | 0,15      | 5204978    | 1,6154  | 186777                     | 55,37%                   | 0,12%              | 36,94%             | 707035                     |
| 6      | Basic Math     | 29,39     | 1394356828 | 1,2795  | 67993852                   | 53,94%                   | 0,05%              | 9,01%              | 258519560                  |
|        | Rijndael Coder | 8,76      | 515538120  | 11,8348 | 393183                     | 44,18%                   | 2,89%              | 46,16%             | 2660056                    |
|        | Susan Corners  | 0,14      | 5079638    | 1,5765  | 186777                     | 55,37%                   | 0,07%              | 57,09%             | 707035                     |
| 7      | Basic Math     | 29,14     | 1382305168 | 1,2684  | 67993852                   | 53,94%                   | 0,01%              | 53,67%             | 258519560                  |
|        | Rijndael Coder | 4,29      | 252271790  | 5,7912  | 393183                     | 44,18%                   | 1,19%              | 48,99%             | 2660056                    |
|        | Susan Corners  | 0,14      | 4962768    | 1,5402  | 186777                     | 55,37%                   | 0,07%              | 54,25%             | 707035                     |
| 8      | Basic Math     | 29,13     | 1381954768 | 1,2681  | 67993852                   | 53,94%                   | 0,01%              | 53,10%             | 258519560                  |
|        | Rijndael Coder | 4,09      | 240490790  | 5,5208  | 393183                     | 44,18%                   | 1,19%              | 46,09%             | 2660056                    |
|        | Susan Corners  | 0,14      | 4936368    | 1,5320  | 186777                     | 55,37%                   | 0,07%              | 52,83%             | 707035                     |
| 9      | Basic Math     | 29,19     | 1423932512 | 1,3066  | 74578944                   | 59,17%                   | 0,30%              | 2,20%              | 238764284                  |
|        | Rijndael Coder | 14,34     | 790550069  | 18,1481 | 495040                     | 55,62%                   | 4,34%              | 49,00%             | 2354485                    |
|        | Susan Corners  | 0,16      | 5107763    | 1,5852  | 186777                     | 64,98%                   | 0,12%              | 36,94%             | 609820                     |
| 10     | Basic Math     | 33,74     | 1735370980 | 1,5924  | 67993852                   | 53,94%                   | 0,30%              | 2,20%              | 550202752                  |
|        | Rijndael Coder | 13,71     | 796069591  | 18,2748 | 393183                     | 44,18%                   | 4,34%              | 49,00%             | 7874007                    |
|        | Susan Corners  | 0,13      | 6034250    | 1,8727  | 186777                     | 55,37%                   | 0,12%              | 36,94%             | 1536307                    |
| 11     | Basic Math     | 37,66     | 1939352536 | 1,7796  | 0                          | 0,00%                    | 0,30%              | 2,20%              | 754184308                  |
|        | Rijndael Coder | 14,28     | 797249140  | 18,3019 | 0                          | 0,00%                    | 4,34%              | 49,00%             | 9053556                    |
|        | Susan Corners  | 0,18      | 6594581    | 2,0466  | 0                          | 0,00%                    | 0,12%              | 36,94%             | 2096638                    |
## Análise

##### Processador com Múltiplos Estágios
Os processadores com múltiplos estágios tiveram uma variação mínima, que corresponde ao número de ciclos que a primeira instrução leva para completar o pipeline. Sendo assim, o número de ciclos do pipeline de 5, foi o número de instruções do simulador somado 4 instruções. Para os pipelines 7 e 13 foram 6 e 12 ciclos adicionais em relação ao simulador com um único estágio.

##### Configurações de Cache
Reunimos os dados de L1 miss rate e L2 miss rate para as configurações 5, 6, 7 e 8, para cada um dos softwares analisados, e construímos os gráficos expostos a seguir. 

![chart](/charts/TotalCiclos.png "MissRate")

Observando esses gráficos, vemos que os misses de L1 foram baixos para as quatro configurações diferentes de cache. Notamos também que os misses de L2 ficaram, em sua maioria, dentro da faixa de 45% e 60%. Adicionalmente, o miss rate de L2 apresentou valores baixos (< 10% ) para a configuração de cache 5 e 8 no software BasicMath.
Em geral, ao comparar essas 4 configurações, nota-se que as configurações 5 e 8 se destacam exatamente por esse comportamento de miss rate de L2 baixo, sendo que todas as outras configurações mostraram-se semelhantes nos outros aspectos. Todavia, pode-se decidir que a configuração 8 teve melhor desempenho, uma vez que apresentou valores menores de L1 miss rate para todos os softwares e que, visto nos gráficos a seguir, teve menor número de ciclos totais. De modo geral, para programas heterogêneos, o aumento no tamanho de cache é o principal responsável pela redução no número de misses.

![chart](/charts/L1xL2.png "L1xL2")

##### Processador Superescalar
Como a configuração 4 difere da 5 apenas em ser superescalar (5 não é), comparamos o comportamento dessas duas configurações: o superescalar apresentou melhores valores para a quantidade de ciclos, isto é, C5 apresentou maior quantidade de ciclos totais (até 15%), mesmo tendo um valor inferior (4x menor) de stalls de dados e controle que C4. Isso indica que o processador superescalar, mesmo efetuando o dobro de instruções por ciclo, pode não ser tão vantajoso, uma vez que aumenta muito a complexidade do processador e traz um ganho razoavelmente pequeno. 

##### Forwarding de Dados
Pontualmente, C10 que se parece muito com C5, diferindo apenas que C10 não possui forwarding, apresentou uma quantidade total de ciclos até 20% maior que C5, e um valor maior no total de stalls (até 2x maior). Analisando os dados medidos na tabela `medidas.ods`, foi possível notar uma redução de 98% dos stalls de dados devido ao uso do forwarding, o que mostra a importância de se usar o forwarding de dados em um processador.

##### Branch Predictor
Por não ter branch predictor, C11, em comparação com C10, possuiu maior quantidade de stalls (cerca de 40% maior) e, por consequência, maior quantidade de ciclos totais (cerca de 10% maior). O branch predictor always not taken da configuração C10 conseguiu acertar de 44 a 55% dos branches nos programas medidos. 
Para C9, que difere de C5 apenas no branch predictor - C9 tem branch predictor 1 bit indicator - vemos que há uma melhora na acurácia do branch predictor. O branch predictor de 1 bit conseguiu prever corretamente cerca de 6-11% mais branches em relação ao branch always taken e teve um total de ciclos menor. O número de ciclos foi reduzido de 1-21% com o uso do 1 bit taken, o que mostra que uma pequena melhora na percentagem de acertos do branch traz uma melhora considerável no número de ciclos.


## Conclusão
Cada parâmetro avaliado mostrou uma melhora considerável no desempenho, que pode ser vista no número de ciclos que o  processador leva para concluir a tarefa. Dentre as medidas analisadas, uma das mais importantes é a adoção de uma configuração de cache adequada, uma vez que o acesso à memória é muito custoso. A simples variação das configurações de cache variaram o número total de ciclos em até 100% no programa rijndael. Além disso, algumas medidas simples, como a adoção de um branch always not taken ou de  forwarding de dados, podem reduzir consideravelmente o número de ciclos. O uso de um branch dinâmico simples, como o 1 bit taken, também pode trazer grande melhora no desempenho, de 1-21% no número de ciclos em relação a um branch always not taken. Por fim, o uso de um pipeline sofisticado pode aumentar muito o número de hazards e tornar seu tratamento complicado e não muito eficiente. O uso de um pipeline superescalar quadriplicou o número de stalls necessários e trouxe uma melhora de desempenho de cerca de 10%, o que representa uma melhora pequena se for comparada com as outras medidas mais simplificadas. 

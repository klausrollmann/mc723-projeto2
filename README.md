# Projeto 2: Desempenho do Processador

Matheus Yokoyama Figueiredo     137036

Klaus Rollmann                  146810

Carlos Gregoreki                104721

## Roteiro

### Benchmarks

* basicmath
* rijndael coder
* susan corners

### Estratégia para critérios básicos

#### Pipeline, escalar e superescalar

Para escolha de tamanho de pipeline será contado o número de ciclos, sem outras modificações.

Quanto ao processador escalar e superescalar, deixaremos o pipeline de tamanho 5 e então contabilizaremos o numero de ciclos para cada um.

#### Hazards

Para analizar os hazards, iremos utilizar pipeline de 5 estágios, adicionando mais ciclos quando ocorrem hazard de dados e controle, simulando o processador sem forwarding e com forwarding de dados.

No processador escalar, poderá ocorrer hazard de dados do tipo RAW. Nesse caso será adicionado um ciclo de stall ou dois, dependendo da dependência. No caso com forwarding, só irá ocorrer hazard com instruções de leitura de memória. Um ciclo adicional será contabilizado nesse caso.
No processador superescalar, haverá hazards de dados do tipo WAR e WAW se duas instruções dependentes estiverem na mesma etapa do ciclo. Nessa caso será contabilizado um ciclo adicional.

Hazards de controle ocorrem quando não tem branch predictor ou quando o branch predictor erra na predição. Nesse caso serão adicionados o número de ciclos dependendo do tipo de branch e branch predictor.

#### Cache
As configurações de cache avaliadas serão as mostradas na tabela abaixo.

| Configuração | L1usize| L1block | Associatividade L1 | L2usize | L2block | Associatividade L2 |
|:------------:|:------:|:-------:|:------------------:|:------:|:-------:|:------------------:|
|       1      |   32   |    64   |          2         |   256  |   1024  |          2         |
|       2      |   64   |   128   |          2         |   512  |   1024  |          2         |
|       3      |   128  |   128   |          2         |  1024  |   2048  |          2         |
|       4      |   128  |   128   |          2         |  1024  |   2048  |          4         |

#### Branch Predictor
Utilizando as configurações anteriores, utilizaremos os seguintes branch predictors:

* Sem branch predictor
* Always not taken
* 1 bit,indicador de taken

### Experimentos

Para avaliar os experimentos, levaremos em consideração os seguintes eventos:
* Stalls de dados e controle
* CPI
* Branch correct predictions
* Cache miss rate
* Ciclos
* Tempo

### Configurações
* Configuração 1: Sem outras modificações, pipeline de 5 estágios escalar.
* Configuração 2: Sem outras modificações, pipeline de 7 estágios escalar.
* Configuração 3: Sem outras modificações, pipeline de 13 estágios escalar.
* Configuração 4: Pipeline 5 estágios superescalar, com forwarding e branch predictor always not taken, e configuração 1 de cache.
* Configuração 5: Pipeline 5 estágios escalar, com forwarding e branch predictor always not taken, e configuração 1 de cache.
* Configuração 6: Pipeline 5 estágios escalar, com forwarding e branch predictor always not taken, e configuração 2 de cache.
* Configuração 7: Pipeline 5 estágios escalar, com forwarding e branch predictor always not taken, e configuração 3 de cache.
* Configuração 8: Pipeline 5 estágios escalar, com forwarding e branch predictor always not taken, e configuração 4 de cache.
* Configuração 9: Pipeline 5 estágios escalar, com forwarding e branch predictor 1 bit indicator, e configuração 1 de cache.
* Configuração 10: Pipeline 5 estágios escalar, sem forwarding e com branch predictor always not taken, e configuração 1 de cache.
* Configuração 11: Pipeline 5 estágios escalar, sem forwarding e sem branch predictor, e configuração 1 de cache.

Dessa forma, cada critério escolhido pode ser avaliado de modo independente.

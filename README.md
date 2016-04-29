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

#### Hazard de dados e controle

Para analizar os hazards, iremos utilizar pipeline escalar de 5 estágios, adicionando mais ciclos quando ocorrem hazard de dados e controle, simulando o processador sem forwarding e com forwarding de dados.

### Cache
Processador: escalar 5 estágios, com forwarding de dados padrão.

| Configuração | L1size | L1block | Associatividade L1 | L2size | L2block | Associatividade L2 |
|:------------:|:------:|:-------:|:------------------:|:------:|:-------:|:------------------:|
|       1      |   32   |    64   |          2         |   256  |   1024  |          2         |
|       2      |   64   |   128   |          2         |   512  |   1024  |          2         |
|       3      |   64   |   128   |          2         |  1024  |   2048  |          2         |
|       4      |   64   |   128   |          2         |  1024  |   2048  |          4         |

### Branch Predictor
Utilizando as configurações anteriores, utilizaremos os seguintes branch predictors:

* Always taken
* 1 bit,indicador de taken

### Experimentos

Para avaliar os experimentos, levaremos em consideração os seguintes eventos:
* Stalls de dados e controle
* CPI
* Branch correct predictions
* Cache miss rate

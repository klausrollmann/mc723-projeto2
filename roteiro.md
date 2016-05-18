# Projeto 2: Desempenho do Processador

Matheus Yokoyama Figueiredo     137036

Klaus Rollmann                  146810

Carlos Gregoreki                104721

## Introdução

## Metodologia
O desempenho do processador foi comparado seguindo quatro pontos principais, são eles:
* Configurações de Pipeline
* Forwarding de Dados
* Configurações de Cache
* Branch Predictor

Para isso, o simulador do processador MIPS foi alterado para contabilizar stalls perdidos por hazards de dados e controle para cada configuração do processador. Além disso, também foram contabilizados stalls devido ao acesso à memória, usando o simulador de cache `dinneroiv`. O processador usado como base nos testes foi o de 5 estágios, e foram feitos testes com 11 configurações distintas para poder avaliar cada um dos quatro fatores que influenciam no desempenho do processador. 

#### Configurações de Pipeline

O pipeline de 5 estágios foi comparado com processadores com 7 e 13 estágios, e também com um processador superescalar de 5 estágios. Assim foi possível verificar tanto a influência do aumento no número de estágios, quanto o efeito de um processador superescalar no desempenho.

<<< processador com múltiplos estágios >>>
 
 No processador superescalar, em cada estágio do pipeline são executadas duas instruções ao mesmo tempo. Por um lado, o processador executa duas instruções por ciclo de clock, o que melhora o número de instruções por segundo, mas o número de hazards também aumenta, e mais stalls são necessários.
 
 #### Hazards e Forwarding de Dados
 
 Os hazards de dados foram contabilizados tanto no processador escalar de 5 estágios como no superescalar. O forwarding de dados foi analisado somente no processador escalar. Os outros processadores foram considerados já com o forwarding de dados.
 
 ##### Forwarding de dados
 Para o processador escalar de 5 estágios, foi contabilizada a influência do forwarding de dados feito na ALU. Isso foi feito fazendo-se três verificações durante a execução do simulador.
1. **Instrução atual lê um valor escrito pela instrução anterior:** Quando isso ocorre e não há forwarding, são adicionados 2 stalls. Com o forwarding não há hazard e não são necessários stalls.
2. **Instrução atual lê um valor escrito pela instrução antes da anterior:** Esse caso é similar ao anterior, porém a instrução já está um estágio distante da anterior, então só é necessário 1 stall caso não houver forwarding. Com o forwarding também não há penalidades nesse caso.
3. **Instrução atual lê um valor que foi escrito por uma instrução de load anterior:** Quando isso ocorre, mesmo com o forwarding, é necessário adicionar um stall, pois a instrução de load só obtém o dado no estágio seguinte em que a instrução atual está.

## Resultados

## Análise

## Conclusão

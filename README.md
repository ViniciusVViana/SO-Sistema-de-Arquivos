# Sistema de Arquivos Simples com Alocação Indexada

Este repositório contém a implementação de um sistema de arquivos simples que utiliza alocação indexada para armazenamento de dados e um mapa de bits para gerenciamento de blocos livres. O sistema foi projetado para operar em um disco simulado (arquivo) ou diretamente em dispositivos como pendrives.
## Conceitos Básicos
### Bloco

- Unidade básica de armazenamento.

- Tamanho especificado pelo usuário.

- Gerenciamento: Cada bloco é gerenciado por um mapa de bits (1 bit por bloco: 0 = livre, 1 = ocupado).

### Índice (Bloco de Índice)

- Cada arquivo/diretório possui um bloco de índice que armazena ponteiros para blocos de dados.

- Capacidade: N entradas (4 bytes cada), dependendo do tamanho dos blocos.

### Diretório

- Tipo especial de arquivo que armazena entradas (nome do arquivo, bloco de índice, tamanho).

- Estrutura: Lista encadeada, com entradas alinhadas em 4 bytes.

## Layout do Disco
|Componente|Descrição|
|---|---|
|Superbloco (Bloco 0)|	Metadados do sistema (tamanho do disco, bitmap, diretório |raiz, etc.).
|Bitmap (Bloco 1 a B)|	Mapa de bits para gerenciar blocos livres.|
|Bloco de Índice do Raiz|	Ponteiros para os blocos que armazenam as entradas do diretório raiz.|
|Blocos de Dados / Entradas|	Blocos restantes para armazenar arquivos/diretórios e entradas do diretório.|
## Estruturas de Dados
### Superbloco (Bloco 0)
|Byte Inicial|	Byte Final|	Tamanho em Bytes|	Campo|	Descrição|
|---|---|---|---|---|
|0|	3|	4|	total_blocks|	Número total de blocos no sistema.|
|4|	7|	4|	bitmap_start|	Bloco inicial do bitmap (sempre 1).|
|8|	11|	4|	bitmap_blocks|	Número de blocos ocupados pelo bitmap.|
|12|	15|	4|	root_dir_index|	Bloco de índice do diretório raiz.|
|16|	19|	4|	free_blocks|	Número de blocos livres restantes.|
|20|	23|	4|	block_size|	log2(tamanho_do_bloco) - log2(512).|
|24|	27|	4|	superblock_number|	Número do bloco que contém o superbloco.|
|28|	31|	4|	version|	Versão do sistema de arquivo.|
### Bitmap

- Estrutura e Mapeamento:

    - Cada bit corresponde a um bloco físico:

        - 0: Bloco livre.

        - 1: Bloco ocupado.

    - Mapeamento lógico-físico:

        - O bit i no bitmap corresponde ao bloco físico i no disco.

        - Exemplo: Bit 0 → Bloco 0 (superbloco), Bit 1 → Bloco 1 (primeiro bloco do bitmap), etc.

    - Armazenamento: O bitmap ocupa blocos consecutivos após o superbloco (bloco 1 em diante).

    - Cálculo do tamanho do bitmap:

        bitmap_blocks = ceil(total_blocks / (block_size * 8))

    - Exemplo: block_size = 512 bytes, total_blocks = 40.000:

        bitmap_blocks = ceil(40.000 / (512 * 8)) = 10

- Algoritmos de Alocação/Liberação:

    - Alocar Bloco Livre:

        - Busca linear: Varre o bitmap até encontrar o primeiro bit 0.

        - Atualização do bitmap:

            - Define o bit encontrado como 1.

            - Atualiza free_blocks no superbloco (free_blocks--).

        - Retorno: Número do bloco alocado.

    - Liberar Bloco:

        - Validação: Verifica se o bloco está dentro do intervalo válido (0 ≤ bloco < total_blocks).

        - Atualização do bitmap:

            - Define o bit correspondente ao bloco como 0.

            - Atualiza free_blocks no superbloco (free_blocks++).

- Exclusão de Arquivo:

    - Remover entrada do diretório:

        - Substituir o primeiro caractere do filename por \0 e definir index_block = ```0xFFFFFFFF```.

    - Liberar blocos de dados:

        - Para cada ponteiro válido no bloco de índice do arquivo:

            - Marcar o bloco como livre no bitmap.

    - Liberar bloco de índice:

        - Marcar o bloco de índice como livre no bitmap.

    - Atualizar metadados:

        - free_blocks += (número de blocos liberados + 1).

### Bloco de Índice do Diretório Raiz

- Estrutura:

    - Tamanho fixo: 1 bloco.

    - N ponteiros para blocos de dados do diretório raiz:

        - Cada ponteiro é um número de bloco físico (ex: block_ptrs[0] = 5 indica que o primeiro bloco de dados do diretório raiz está no bloco 5).

        - Entradas não utilizadas são marcadas com ```0xFFFFFFFF```.

- Localização:

    - Pré-alocado durante a formatação:

        - O superbloco (campo root_dir_index) armazena o número deste bloco (ex: root_dir_index = 2).

        - Fica posicionado logo após o bitmap (bloco B + 1, onde B é o número de blocos do bitmap).

- Função:

    - Permite acesso às entradas do diretório raiz, que definem arquivos e subdiretórios no nível mais alto.

### Diretório Raiz

- Estrutura:

    - Bloco de índice: Apontado por root_dir_index no superbloco.

    - Blocos de Entradas do Diretório: Alocados dinamicamente conforme necessário, cada um armazenando N(tamanho_do_bloco / 64) entradas de diretório.

- Expansão Dinâmica:

    - Quando as entradas excedem a capacidade dos blocos de dados atuais, o sistema:

        - Busca um bloco livre no bitmap.

        - Adiciona o novo bloco à lista block_ptrs do bloco de índice do diretório raiz.

### Entradas do Diretório Raiz

Cada entrada no diretório raiz descreve um arquivo ou subdiretório.
|Byte Inicial|	Byte Final|	Tamanho em Bytes|	Campo|	Descrição|
|---|---|---|---|---|
|0|	54|	55|	filename|	Nome do arquivo/diretório (54 caracteres + \0).|
|55|	55|	1|	file_type|	Tipo de arquivo (arquivo, diretório, etc).|
|56|	59|	4|	index_block|	Número do bloco de índice associado ao arquivo/diretório.
|60|	63|	4|	file_size|	Tamanho do arquivo em bytes (ignorado para diretórios).
### Índices de Dados do Arquivo

Cada arquivo possui um bloco de índice que gerencia seus dados. O bloco de índice possui ```N - 1``` ponteiros para os blocos de dados e um ponteiro para outro bloco de índice adicional.
|Byte Inicial|	Byte Final|	Tamanho em Bytes|	Campo|	Descrição|
|---|---|---|---|---|
|0|	N - 1|	4*(N-1)|	```block_ptrs[N]```|	N ponteiros para os blocos de dados (ex: bloco com 512 bytes, 512 / 4 = 128 ponteiros de 4 bytes cada).|
|4*(N-1)|	N|	4|	```indirect_ptr```|	Ponteiro para um bloco de índice adicional.|

- Expansão de Arquivos:

    - Se todos os ponteiros diretos estiverem ocupados, o último ponteiro do bloco de índice aponta para um novo bloco de índice.

    - O novo bloco de índice segue a mesma estrutura, permitindo expansão do arquivo.

## Fluxo de Acesso

    - Superbloco: Define a localização do diretório raiz (root_dir_index).

    - Bloco de índice do diretório raiz: Aponta para blocos de dados com entradas.

    - Entrada no diretório raiz: Fornece o index_block do arquivo desejado.

    - Bloco de índice do arquivo: Aponta para os blocos de dados do arquivo.
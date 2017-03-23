#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

// PARA DESABILITAR AS OPCOES, COMENTE O "#define"
// MODIFIQUE AQUI O NUMERO DE THREADS
#define THREAD_NUMBER 2
// CAMINHO ORIGINAL DO TEXTO
#define FILE_DIR "bible.txt"
// MODO DE DEPURACAO
//#define DEBUG true
// USAR VERIFICACAO DE TAMANHO
#define USE_LENGTH_ANALYSIS true
// TAMANHO MAXIMO CASO UTILIZE "USE_LENGTH" 
#define MINIMAL_LENGTH 2
// MOSTRA LISTA DE PALINDROMOS
#define SHOW_PALINDROME_LIST true

// GUARDA A INFORMACAO DO ARQUIVO ABERTO
struct file_information {
    // TAMANHO DO ARQUIVO
    size_t size;
    // BUFFER COM O CONTEUDO DO ARQUIVO
    char * content;
};
typedef struct file_information f_info;

// ESTRUTURA QUE GUARDA UMA VARIAVEL E SEU MUTEX
struct mutex_package {
    // ALVO DO MUTEX
    int * target;
    // CONTROLE DE ACESSO POR MUTEX
    pthread_mutex_t * mutex;
};
typedef struct mutex_package m_pack;

// ESTRUTURA QUE CONTEM TODOS OS DADOS NECESSARIOS PARA A THREAD
struct thread_package {
    // INDETIFICADOR DA THREAD
    size_t id;
    // FLAG DE ULTIMA THREAD
    bool is_last_thread;
    // CARGA DE TAMANHO DA THREAD
    size_t work_size;
    // FLAG PARA AS INFORMACOES DO ARQUIVO
    f_info * file;
    // FLAG PARA ACUMULADOR DE QUANTIDADE DE PALINDROMOS
    m_pack * palindrome;
};
typedef struct thread_package t_pack;

// INICIALIZADOR DE M_PACK
// @param m_pack * newest objeto que sera iniciado
// @param void * target variavel que sera encapsulada
void mutex_package_init(m_pack * newest, void * target) {
    // INICIALIZA MUTEX ESTATICO
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    // ENCAPSULA MUTEX
    newest->mutex = &mutex;
    // ENCAPSULA VARIVAVEL
    newest->target = target;
}

// INICIALIZADOR DE T_PACK
// @param t_pack * newest objeto que sera iniciado
// @param f_info * informacao do arquivo que sera encapsulado
// @param size_t id identificador unico da thread
// @param work_size numero de caracteres que a thread analizara
// @param m_pack * variavel que sera acessa por todas sa threads
void thread_package_init(t_pack * newest, f_info * file, size_t id, size_t work_size, m_pack * palindrome) {
    // ENCAPSULAMENTO
    newest->file = file;
    newest->id = id;
    newest->work_size = work_size;
    newest->palindrome = palindrome;
}

// RECUPERA INFORMACOES DE UM ARQUIVO 
// @param char * dir local relativo do arquivo
// @return f_info * ponteiro para as informacoes do arquivo
f_info * get_file_content(char * dir) {
    // ABRE ARQUIVO 
    FILE * f_ptr = fopen(dir, "r");
    if (f_ptr) {
        // ACHA O FINAL DO ARQUIVO
        fseek(f_ptr, 0, SEEK_END);
        // CRIA A ESTRUTURA 
        f_info * file = (f_info *) malloc(sizeof (f_info));
        // TAMANHO DO ARQUIVO
        file->size = ftell(f_ptr);
        // CRIA UM BUFFER COM O TAMANHO DO ARQUIVO
        file->content = (char *) malloc(sizeof (char) * file->size);
        // ACHA O INICIO DO ARQUIVO
        fseek(f_ptr, 0, SEEK_SET);
        // ENVIA TODO O CONTEUDO DO ARQUIVO PARA O BUFFER
        fread(file->content, file->size, 1, f_ptr);
        return file;
    }
    perror("Falha ao abrir arquivo.");
    exit(-1);
    return NULL;
}

// EXECUTA VERIFICACAO DE PALINDROMO
// @param char * start ponteiro inicial da palavra
// @param size_t size tamanho da palavra
// @return bool resposta
bool palindrome(char * start, size_t size) {
    // ACHA O MEIO DO ARQUIVO
    size_t middle = size / 2;
    // ACHA O PONTEIRO PARA O FINAL DA PALAVRA
    char * end = start + (size - 1);
    while (middle > 0) {
        // VERIFICA
        if (*start != *end) {
            return false;
        }
        start++;
        end--;
        middle--;
    }
    return true;
}

// TRANSFORMA PALAVRA EM SINGULAR
// @param char * input palavra 
// @param size_t length tamanho da palavra
// @return char * ponteiro para a propria palavra
char * lower_case(char * input, size_t length) {
    size_t i = 0;
    for (i = length; i > 0; i--) {
        if (*(input + i) >= 65 && *(input + i) <= 90) {
            *(input + i) += 32;
        }
    }
    return input;
}

// CONTEUDO QUE SERA EXECUTADO PELA THREAD
// @param void * arg parametro de entrada
void * thread_callback(void * arg) {
    t_pack * param = arg;
    // FINAL DO BUFFER QUE SERA ANALISADO
    size_t to = (param->work_size * (param->id + 1));
    // INICIO DO BUFFER QUE SERA ANALISADO
    size_t from = to - param->work_size;
    // CASO SEJA A ULTIMA THREAD, ANALIZA ATE O FINAL DO ARQUIVO
    if (param->is_last_thread) {
        to = param->file->size;
    }
    char * backup = NULL;
    char * buffer = NULL;
    int acc = 0;
    
    // RECUPERA PRIMEIRA PALAVRA
    buffer = __strtok_r(param->file->content + from, " ,.-\n\0", (char **) &backup);
    
    // ENQUANTO ESTIVER DENTRO DA FAIXA DETERMINADA
    while (buffer != NULL && buffer < param->file->content + to) {
        // RECUPERA TAMANHO DA PALAVRA
        size_t length = strlen(buffer);
        
        // SE A PALAVRA FOR PALINDRO
        // CASO USE VERIFICACAO DE TAMANHO DE PALAVRA
        #ifdef USE_LENGTH_ANALYSIS
            if (palindrome(lower_case(buffer, length), length) && length > MINIMAL_LENGTH) {
        // CASO NAO UTILIZE ESSA VERIFICACAO
        #else
            if (palindrome(lower_case(buffer, length), length)) {
        #endif
            // MOSTRA PALINDROMO
            #ifdef SHOW_PALINDROME_LIST
                printf("%s \t", buffer);
            #endif
            // INCREMENTA BUFFER
            acc++;
        }
        // PROXIMA PALAVRA
        buffer = __strtok_r(NULL, " :,;.-\n\t\r#", &backup);
    }

    // TRANCA MUTEX DO ACUMULADOR COMPARTILHADO
    pthread_mutex_lock(param->palindrome->mutex);
    // ACUMULACAO
    *param->palindrome->target += acc;
    // LIBERA MUTEX DO ACUMULADOR COMPARTILHADO
    pthread_mutex_unlock(param->palindrome->mutex);
    #ifdef DEBUG
        // MOSTRA DADOS DE DEPURACAO
        printf("A thread %u recuperou %d palindromos\n", (unsigned int)param->id, acc);
    #endif
    // FINALIZA THREAD
    pthread_exit(NULL);
}

int main() {
    // RECUPERA ARQUIVO
    f_info * file = get_file_content(FILE_DIR);
    // RESTO DA DIVISAO DE TRABALHO ENTRE AS THREADS
    size_t mod = file->size % THREAD_NUMBER;
    // TRABALHO QUE SERA DIVIDO ENTRE AS THREADS
    size_t work_per_thread = file->size / THREAD_NUMBER;
    // INICIANDO VARIAVEIS
    size_t i = 0; 
    static size_t palindrome = 0;
    m_pack m_palindrome;
    
    // INICIA PACOTE DE MUTEX
    mutex_package_init(&m_palindrome, &palindrome);
    
    // ALOCACAO DINAMICA
    pthread_t * t = (pthread_t *) malloc(sizeof (pthread_t) * THREAD_NUMBER);
    t_pack * per_thread = (t_pack *) malloc(sizeof (t_pack) * THREAD_NUMBER);
    
    #ifdef DEBUG
        printf("O arquivo tem %u letras.\n", (unsigned int)file->size);
    #endif // DEBUG

    // PARA CADA THREAD
    for (i = 0; i < THREAD_NUMBER; i++) {
        // INICIA O PACOTE DE THREAD
        thread_package_init(per_thread + i, file, i, work_per_thread, &m_palindrome);
        // MARCA FLAG CASO SEJA A ULTIMA THREAD
        (per_thread + i)->is_last_thread = (i == THREAD_NUMBER - 1 && mod != 0);
        // CRIA THREAD
        pthread_create(t + i, NULL, thread_callback, per_thread + i);
    }
    
    // DA JOIN EM CADA THREAD
    for (i = 0; i < THREAD_NUMBER; i++) {
        pthread_join(*(t + i), NULL);
    }
    
    // MOSTRA TOTAL DE PALINDROMOS
    printf("PALINDROMOS %u\n", (unsigned int) palindrome);
}



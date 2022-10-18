#include "threads.h"
#include <pthread.h>
#include <stdlib.h>

int t_id[num_threads];


int main(int argc, char **argv)
{
    srand(time(NULL));
    
    pthread_mutex_init(get_address(), NULL);
    pthread_barrier_init(get_barrier(),NULL, num_threads);

    pthread_t gui_th, gendata_th, locate_th[num_threads];
    pthread_create(&gui_th, NULL, GUI, NULL);
    pthread_create(&gendata_th, NULL,generate_data , NULL);
    //open_mp_for();
    //locate_open_mp();
    
//  pthread  

    for(int i=0; i<num_threads; i++){
        t_id[i] = i;
        pthread_create(&locate_th[i], NULL, locate_object, &(t_id[i]));
    }
    
    for(int i=0; i<num_threads; i++){
        pthread_join(locate_th[i],NULL);
    }
    pthread_join(gui_th, NULL);
    pthread_join(gendata_th, NULL);
}

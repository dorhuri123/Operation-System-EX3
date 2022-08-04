//Dor Huri - 209409218
#include <iostream>
#include <semaphore.h>
#include <mutex>
#include <thread>
#include <queue>
#include <utility>
#include <fstream>
#include <string>
#define DONE "DONE"
#define SPORTS "SPORTS"
#define NEWS "NEWS"
#define WEATHER "WEATHER"
using namespace std;
using namespace this_thread;
using namespace chrono;
//bounded class
class BoundedBuffer {
private:
    queue<string> q;
    sem_t full{};
    sem_t empty{};
    mutex m;

public:
    explicit BoundedBuffer(int size) {
        //initializing semaphore
        sem_init(&full, 0, 0);
        sem_init(&empty, 0, size);
    }
    //for inserting to bounded queue
    void insert(const string& s){
        //decrease by 1 or block
        sem_wait(&empty);
        //lock thread queue
        m.lock();
        q.push(s);
        //unlock thread queue
        m.unlock();
        //increase by 1
        sem_post(&full);
    }
    //for removing of bounded queue
    string remove(){
        //decrease by 1 or block
        sem_wait(&full);
        //lock thread queue
        m.lock();
        //save string and remove from queue
        string s = q.front();
        q.pop();
        //unlock thread queue
        m.unlock();
        //increase by 1
        sem_post(&empty);
        return s;
    }
    //bounded destructor
    ~BoundedBuffer() {
        sem_destroy(&full);
        sem_destroy(&empty);
    }
};
//unbounded class
class UnBoundedBuffer {
private:
    queue<string> q;
    sem_t sem{};
    mutex m;

public:
    UnBoundedBuffer() {
        //initializing semaphore
        sem_init(&sem, 0, 0);
    }
    //for inserting to unbounded queue
    void insert(const string& s){
        //lock thread queue
        m.lock();
        q.push(s);
        //unlock thread queue
        m.unlock();
        //increase by 1
        sem_post(&sem);
    }
    //for removing of unbounded queue
    string remove(){
        //decrease by 1 or block
        sem_wait(&sem);
        //lock thread queue
        m.lock();
        //save string and remove from queue
        string s = q.front();
        q.pop();
        //unlock thread queue
        m.unlock();
        return s;
    }
    //unbounded destructor
    ~UnBoundedBuffer() {
        sem_destroy(&sem);
    }
};
//struct for passing argument to Producer
struct Producer {
    int id;
    int sum;
    BoundedBuffer *bf;
} typedef Producer;
//struct for passing argument to Dispatcher
struct Dispatcher {
    vector<BoundedBuffer*> boundedList;
    UnBoundedBuffer* sport;
    UnBoundedBuffer* news;
    UnBoundedBuffer* weather;
} typedef Dispatcher;
//struct for passing argument to coEditors
struct CoEditors {
    UnBoundedBuffer* ubf;
    BoundedBuffer* bf;
} typedef CoEditors;

/******************************************************************************
 * the function is for the producer in this function the producer creates his articles
 * and save them in his queue for the dispatcher to sort and pass on.
 ******************************************************************************/
void * produce(void* prod){
    int count_buf[3] ={};
    string category[3] = {SPORTS,NEWS,WEATHER};
    int i =0;
    //casting argument
    auto* p = (Producer*)prod;
    //loop for creating all article
    for(;i < p->sum; i++){
        sleep_for(milliseconds(100));
        int num = rand()%3;
        //creating article
        string str = "Producer " + to_string(p->id) +  " " + category[num] + " " + to_string(count_buf[num]);
        //updating counter and inserting article to queue
        count_buf[num]++;
        p->bf->insert(str);
    }
    // indicate producer finish
    p->bf->insert(DONE);
    return (void*)nullptr;
}
/******************************************************************************
 * the function is for the dispatcher where the dispatcher get all the article
 * from the producer(using red robin) and sort each to the appropriate
 * unbounded buffer.
 ******************************************************************************/
void* dispatchers(void* disp){
    //casting argument
    auto* d = (Dispatcher *) disp;
    int num_of_product = (int)d->boundedList.size(),size = num_of_product;
    //while not all the article from producer are finish
    while(size) {
        int i =0;
        //loop in red robin
        for(; i < num_of_product; i++) {
            //if nullptr continue
            if(d->boundedList[i] == nullptr) {
                continue;
            }
            //saving the article string
            string res = d->boundedList[i]->remove();
            //if the string is "DONE" meaning the specific producer finish produce
            if(res == DONE) {
                d->boundedList[i] = nullptr;
                //decreasing counter by 1
                size--;
            }
            //sorting the article to correct queue by type
            if(res.find(SPORTS) != std::string::npos) {
                d->sport->insert(res);
            }
            if(res.find(NEWS) != std::string::npos) {
                d->news->insert(res);
            }
            if(res.find(WEATHER) != std::string::npos) {
                d->weather->insert(res);
            }
        }
    }
    //indicating the dispatcher has finish
    d->sport->insert(DONE);
    d->news->insert(DONE);
    d->weather->insert(DONE);
    return (void*)nullptr;
}
/******************************************************************************
 * the function is for the sco editors so they can edit the article that was
 * sorted by the dispatcher before passing it to the screen manager to print.
 ******************************************************************************/
void* edit(void* coEdit){
    //casting argument
    auto co = (CoEditors *)coEdit;
    while (true){
        //removing article from dispatcher queue
        string res = co->ubf->remove();
        sleep_for(milliseconds(100));
        //inserting to screen queue
        co->bf->insert(res);
        //if we get "DONE" exit loop
        if(res == DONE){
            break;
        }
    }
    return (void *)nullptr;
}
/******************************************************************************
 * the function is for the screen manger for printing to the screen what the co
 * editors insert to the share bounded buffer they holds.
 ******************************************************************************/
void * manage(void* screen_buf){
    auto s_m = (BoundedBuffer *)screen_buf;
    int num_done = 0;
    //while the screen didn't get all 3 done statement from the co's
    while(num_done != 3){
        //removing article from screen queue
        string res = s_m->remove();
        //if the string is not the done statement print him
        if(res!=DONE)
            cout << res << endl;
        else
            //increase counter
            num_done++;
    }
    //indicating all article are printed
    cout << DONE << endl;
    return (void *)nullptr;
}
/******************************************************************************
 * the function is for parsing  the file according to the assignment instruction
 * in this project i selected option 2 of format file.
 * the function return the co's and screen mutual buffer size.
 ******************************************************************************/
int format_file(const string& path,vector<pair<int,pair<int,int>>>& v){
    fstream conf_file;
    // open a file to perform write operation using file object
    conf_file.open(path,ios::in);
    string producerId,producerSum,producerQueue,emptyLine;
    //checking whether the file is open
    if (conf_file.is_open()){
        //getting the file lines 3 at a time
        while(getline(conf_file, producerId) && getline(conf_file, producerSum) && getline(conf_file, producerQueue)){
            //insert to vector the producer argument
            v.emplace_back(stoi(producerId),make_pair(stoi(producerSum),stoi(producerQueue)));
            //for skipping the empty line
            getline(conf_file, emptyLine);
        }
        //close the file object
        conf_file.close();
        //return the co's and screen mutual buffer size
        return stoi(producerId);
    }
    //in case there wasn't a conf file exit
    else
        exit(1);

}
int main(int argc, char *argv[]) {
    //checking if there are enough arguments
    if(argc != 2){
        cout << "Not Enough Parameter" << endl;
        exit(1);
    }
    pthread_t tid;
    vector<pair<int,pair<int,int>>> v;
    //get the input arguments from file by the format
    int coEditorSize = format_file(argv[1],v),i=0,size=(int)v.size();
    //vector of the bounded buffer of producers
    vector<BoundedBuffer*> bounded_producer;
    //vector for setting producer arguments
    vector<Producer> producer_vec;
    for(; i < size; i++) {
        //setting producer bounded buffer
        bounded_producer.push_back(new BoundedBuffer(v[i].second.second));
        //setting producer threads arguments
        producer_vec.push_back({v[i].first, v[i].second.first, bounded_producer[i]});
    }
    //calling produce threads
    for(i = 0; i < size; i++) {
        pthread_create(&tid, nullptr, &produce, (void*)&producer_vec[i]);
    }
    //creating mutual buffer of dispatcher and co's
    auto sport = new UnBoundedBuffer();
    auto news = new UnBoundedBuffer();
    auto weather = new UnBoundedBuffer();
    //setting dispatcher arguments
    Dispatcher d = {bounded_producer, sport, news, weather};
    //calling dispatcher threads
    pthread_create(&tid, nullptr, &dispatchers, (void*)&d);
    //creating the mutual bounded buffer of the co and screen manager
    auto coEditorQueue = new BoundedBuffer(coEditorSize);
    //creating argument for coEditors
    CoEditors co_s = {sport, coEditorQueue};
    CoEditors co_n = {news, coEditorQueue};
    CoEditors co_w = {weather, coEditorQueue};
    //calling coEditors threads
    pthread_create(&tid, nullptr, &edit, (void*)&co_s);
    pthread_create(&tid, nullptr, &edit, (void*)&co_n);
    pthread_create(&tid, nullptr, &edit, (void*)&co_w);
    //calling screen manager thread
    pthread_create(&tid, nullptr, &manage, (void*)coEditorQueue);
    //wait for threads to finish
    pthread_join(tid,nullptr);
}
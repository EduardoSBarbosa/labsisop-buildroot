#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/ktime.h>

struct sstf_data {
    struct list_head queue;
    unsigned long long current_disk_pos;
    unsigned long long total_seek;
};



static void sstf_add_request(struct request_queue *q, struct request *rq)
{
	struct sstf_data      *sd = q->elevator->elevator_data;

    u64 now_ns = ktime_get_ns();
    
    rq->elv.priv[0] = (void *)(unsigned long)(now_ns & 0xFFFFFFFF); 
    rq->elv.priv[1] = (void *)(unsigned long)(now_ns >> 32);

	list_add_tail(&rq->queuelist, &sd->queue);
}

static int sstf_dispatch_request(struct request_queue *q, int force)
{
    struct sstf_data *sd = q->elevator->elevator_data;
    struct request *rq;
    struct request *best_rq = NULL; /* Nome corrigido e inicializado */
    struct list_head *pos;
    
    /* Declaração das variáveis ausentes */
    unsigned long long req_pos;
    unsigned long long current_dist;
    unsigned long long min_dist = ~0ULL; /* Inicializa com o valor MÁXIMO possível */
    
    if (list_empty(&sd->queue)) {
        return 0;
    }
    
    list_for_each(pos, &sd->queue) {
        rq = list_entry(pos, struct request, queuelist);
        req_pos = (unsigned long long)blk_rq_pos(rq);

        /* Cálculo da distância com o operador ternário corrigido */
        current_dist = (req_pos > sd->current_disk_pos) 
                       ? (req_pos - sd->current_disk_pos) 
                       : (sd->current_disk_pos - req_pos);

        if (current_dist <= min_dist) {
            min_dist = current_dist;
            best_rq = rq;
        }
    }

    if (best_rq) {
        u32 lower = (u32)(unsigned long)best_rq->elv.priv[0];
        u32 upper = (u32)(unsigned long)best_rq->elv.priv[1];
        
        u64 arrival_ns = ((u64)upper << 32) | lower;

        u64 elapsed_time_ms = ktime_get_ns() - arrival_ns;
        sd->total_seek += min_dist;

        sd->current_disk_pos = blk_rq_pos(best_rq) + blk_rq_sectors(best_rq);

        printk(KERN_INFO "SSTF [OUT]: Dispatching sector %llu | Seek distance: %llu | ElapsedTime %llu\n", 
               (unsigned long long)blk_rq_pos(best_rq), min_dist, elapsed_time_ms);

        list_del_init(&best_rq->queuelist);
        
        elv_dispatch_add_tail(q, best_rq);
        
        return 1;
    }

    return 0;
}

static int sstf_init_queue(struct request_queue *q, struct elevator_type *e)
{
    struct sstf_data *sd;
    struct elevator_queue *eq;

    eq = elevator_alloc(q, e);
    if (!eq)
        return -ENOMEM;

    sd = kmalloc_node(sizeof(*sd), GFP_KERNEL, q->node);
    if (!sd){
        kobject_put(&eq->kobj);
        return -ENOMEM;
    }
    
    
    INIT_LIST_HEAD(&sd->queue);
    
    sd->current_disk_pos = 0;
    sd->total_seek = 0;
    eq->elevator_data = sd;
    
    q->elevator = eq;
    
    printk(KERN_INFO "SSTF: Module initialized.\n");
    return 0;
}

static void sstf_exit_queue(struct elevator_queue *e)
{
    struct sstf_data *sd = e->elevator_data;
    
    printk(KERN_INFO "SSTF: Module exited. Total distance traveled (seek): %llu sectors\n", sd->total_seek);
    
    BUG_ON(!list_empty(&sd->queue));
    
    kfree(sd);
}

static struct elevator_type elevator_sstf = {
    .ops.sq = {
        .elevator_add_req_fn  = sstf_add_request,
        .elevator_dispatch_fn = sstf_dispatch_request,
        .elevator_init_fn     = sstf_init_queue,
        .elevator_exit_fn     = sstf_exit_queue,
    },
    .elevator_name = "sstf",
    .elevator_owner = THIS_MODULE,
};

static int __init sstf_init(void)
{
    return elv_register(&elevator_sstf);
}

static void __exit sstf_exit(void)
{
    elv_unregister(&elevator_sstf);
}

module_init(sstf_init);
module_exit(sstf_exit);

MODULE_AUTHOR("Max Breuel");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Shortest search time first I/O sched intrumented");
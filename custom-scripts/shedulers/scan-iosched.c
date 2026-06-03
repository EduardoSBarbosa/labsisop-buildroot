#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/ktime.h>

struct scan_benchmark{
    u64 time_elapsed;
};

struct scan_data {
    struct list_head queue;
    sector_t head_pos;
    int direction; /* 1 = UP, 0 = DOWN */
    unsigned long long total_seek;
};

static int scan_init_queue(struct request_queue *q, struct elevator_type *e)
{
    struct scan_data *sd;
    struct elevator_queue *eq;

    eq = elevator_alloc(q, e);
    if (!eq)
        return -ENOMEM;

    sd = kmalloc_node(sizeof(*sd), GFP_KERNEL, q->node);
    if (!sd) {
        kobject_put(&eq->kobj);
        return -ENOMEM;
    }
    

    INIT_LIST_HEAD(&sd->queue);
    sd->head_pos = 0;
    sd->direction = 1;
    sd->total_seek = 0;

    eq->elevator_data = sd;

    spin_lock_irq(q->queue_lock);
    q->elevator = eq;
    spin_unlock_irq(q->queue_lock);
    
    printk(KERN_INFO "SCAN: Module initialized.\n");
    return 0;
}

static void scan_exit_queue(struct elevator_queue *e)
{
    struct scan_data *sd = e->elevator_data;
    
    printk(KERN_INFO "SCAN: Module exited. Total seek distance: %llu sectors\n", sd->total_seek);
    
    BUG_ON(!list_empty(&sd->queue));
    kfree(sd);
}

static void scan_enqueue_request(struct request_queue *q, struct request *rq)
{
    struct scan_data *sd = q->elevator->elevator_data;
    struct list_head *pos;
    struct scan_benchmark *sb;

    sb = kmalloc(sizeof(*sb), GFP_ATOMIC);

    if (sb){
        sb->time_elapsed = ktime_get_ns();
        rq->elv.priv[0] = sb;
    } else {
        rq->elv.priv[0] = NULL;
    }  
          

    printk(KERN_INFO "SCAN [IN]: Request arriving for sector %llu\n", 
           (unsigned long long)blk_rq_pos(rq));

    list_for_each(pos, &sd->queue) {
        struct request *tmp = list_entry(pos, struct request, queuelist);
        if (blk_rq_pos(rq) < blk_rq_pos(tmp))
            break;
    }

    list_add_tail(&rq->queuelist, pos);
}

static int scan_dispatch_requests(struct request_queue *q, int force)
{
    struct scan_data *sd = q->elevator->elevator_data;
    struct request *rq = NULL, *tmp;
    struct list_head *pos;
    unsigned long long dist;

    if (list_empty(&sd->queue))
        return 0;

    if (sd->direction == 1) { /* UP */
        list_for_each(pos, &sd->queue) {
            tmp = list_entry(pos, struct request, queuelist);
            if (blk_rq_pos(tmp) >= sd->head_pos) {
                rq = tmp;
                break;
            }
        }
        
        if (!rq) {
            sd->direction = 0;
            printk(KERN_INFO "SCAN [CHANGE]: End of path. Reversing direction to DOWN.\n");
            rq = list_entry(sd->queue.prev, struct request, queuelist);
        }
    } else { /* DOWN */
        list_for_each_prev(pos, &sd->queue) {
            tmp = list_entry(pos, struct request, queuelist);
            if (blk_rq_pos(tmp) <= sd->head_pos) {
                rq = tmp;
                break;
            }
        }
        
        if (!rq) {
            sd->direction = 1;
            printk(KERN_INFO "SCAN [CHANGE]: End of path. Reversing direction to UP.\n");
            rq = list_entry(sd->queue.next, struct request, queuelist);
        }
    }

    if (rq) {
        struct scan_benchmark *sb  = rq->elv.priv[0];
        u64 elapsed_time_ms = -1;

        if (sb){
            elapsed_time_ms = ktime_get_ns() - sb->time_elapsed;
            kfree(sb);
            rq->elv.priv[0] = NULL;
        }

        if (blk_rq_pos(rq) > sd->head_pos)
            dist = blk_rq_pos(rq) - sd->head_pos;
        else
            dist = sd->head_pos - blk_rq_pos(rq);
            
        sd->total_seek += dist;
        
        printk(KERN_INFO "SCAN [OUT]: Dispatching sector %llu | Dir: %s | Seek distance: %llu | ElapsedTime %llu\n", 
               (unsigned long long)blk_rq_pos(rq), 
               sd->direction == 1 ? "UP" : "DOWN",
               dist,
               elapsed_time_ms);

        sd->head_pos = blk_rq_pos(rq) + blk_rq_sectors(rq);
        list_del_init(&rq->queuelist);
        elv_dispatch_add_tail(q, rq);
        return 1; 
    }

    return 0;
}

static struct elevator_type elevator_scan = {
    .ops.sq = {
        .elevator_add_req_fn  = scan_enqueue_request,
        .elevator_dispatch_fn = scan_dispatch_requests,
        .elevator_init_fn     = scan_init_queue,
        .elevator_exit_fn     = scan_exit_queue,
    },
    .elevator_name = "scan",
    .elevator_owner = THIS_MODULE,
};

static int __init scan_init(void)
{
    return elv_register(&elevator_scan);
}

static void __exit scan_exit(void)
{
    elv_unregister(&elevator_scan);
}

module_init(scan_init);
module_exit(scan_exit);

MODULE_AUTHOR("Max Breuel");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SCAN/Elevator I/O Scheduler");
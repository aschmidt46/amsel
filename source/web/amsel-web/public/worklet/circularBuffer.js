export class CircularQueueNonBlocking {
  constructor(capacity, evictionCallback) {
    // Validate capacity
    if (typeof capacity !== 'number' || capacity < 1) {
      throw new Error('Please specify capacity of the Queue')
    }

    // Validate optional eviction callback function
    if (!!evictionCallback && typeof evictionCallback !== 'function') {
      throw new Error('evictionCallback should be a function')
    }

    this.head = 0
    this.tail = 0
    this.size = 0
    this.items = Array.from({ length: capacity })
    this.capacity = capacity
    this.evictionCallback = evictionCallback || null
  }

  enqueue(item) {
    if (this.isFull()) {
      // if it is full, use _rotateHead method to remove the current head value
      // and move the head pointer one step forward:
      const removedItem = this._rotateHead()
      // pass the removedItem into evictionCallback if it is present:
      if (this.evictionCallback) {
        this.evictionCallback(removedItem)
      }

      // add the new item:
      this.items[this.tail] = item
      
      // move the tail one step forward for next insertion in advance:
      this.tail = (this.tail + 1) % this.capacity
    } else {
      // if it is not full, attach the item to current tail, then move the tail one step forward:
      this.items[this.tail] = item
      this.tail = (this.tail + 1) % this.capacity
      this.size++
    }
  }

  dequeue() {
    if (this.isEmpty()) throw new Error('Queue is empty!')

    // store the item about to be removed:
    const removedItem = this.peek()

    // set the current head to be undefined (cleanup)
    this.items[this.head] = undefined

    // move the head index by 1 using modulus operator:
    this.head = (this.head + 1) % this.capacity
    this.size--
    return removedItem
  }
  
  peek() {
    return this.items[this.head]
  }
  
    // helper method for overwriting the oldest item in the queue
  _rotateHead() {
    // store the item that is about to be evicted from queue:
    const removedItem = this.peek()
    // move the head to the next position (next oldest item in the list) using modulus logic
    this.head = (this.head + 1) % this.capacity
    // return the removedItem:
    return removedItem
  }

  isFull() {
    return this.capacity === this.size
  }

  isEmpty() {
    return this.size === 0
  }
  
  length() {
    return this.size
  }
  
  clear() {
    this.items = []
    this.head = 0
    this.tail = 0
    this.size = 0
  }
  
  toArray() {
    return this.items
  }
}

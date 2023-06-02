#include <stdint.h>
#include <stdbool.h>
int get_bitset_len(int bitNum){
  return (bitNum+63)>>6;
}
void bitset_modify(uint64_t *bitset,int index,bool val){
  bool oldVal=(bitset[index>>6]>>(index&63))&1;
  if(oldVal!=val){
    bitset[index>>6]^=(1ul<<(index&63));
  }
}
bool bitset_get(uint64_t *bitset,int index){
  return (bitset[index>>6]>>(index&63))&1;
}
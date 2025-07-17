#ifndef GDSCRIPT_SHUNT_H
#define GDSCRIPT_SHUNT_H

typedef struct token_table_t token_table_t;

void gdscript_shunt(token_table_t* dest, const token_table_t* src);

#endif /* End GDSCRIPT_SHUNT_H */

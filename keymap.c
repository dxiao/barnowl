#include <string.h>
#include "owl.h"

static void _owl_keymap_format_bindings(const owl_keymap *km, owl_fmtext *fm);
static void _owl_keymap_format_with_parents(const owl_keymap *km, owl_fmtext *fm);

/* returns 0 on success */
int owl_keymap_init(owl_keymap *km, const char *name, const char *desc, void (*default_fn)(owl_input), void (*prealways_fn)(owl_input), void (*postalways_fn)(owl_input))
{
  if (!name || !desc) return(-1);
  if ((km->name = g_strdup(name)) == NULL) return(-1);
  if ((km->desc = g_strdup(desc)) == NULL) return(-1);
  if (0 != owl_list_create(&km->bindings)) return(-1);
  km->parent = NULL;
  km->default_fn = default_fn;
  km->prealways_fn = prealways_fn;
  km->postalways_fn = postalways_fn;
  return(0);
}

/* note that this will free the memory for the bindings! */
void owl_keymap_cleanup(owl_keymap *km)
{
  g_free(km->name);
  g_free(km->desc);
  owl_list_cleanup(&km->bindings, (void (*)(void *))owl_keybinding_delete);
}

void owl_keymap_set_parent(owl_keymap *km, const owl_keymap *parent)
{
  km->parent = parent;
}

/* creates and adds a key binding */
int owl_keymap_create_binding(owl_keymap *km, const char *keyseq, const char *command, void (*function_fn)(void), const char *desc)
{
  owl_keybinding *kb, *curkb;
  int i;

  if ((kb = g_new(owl_keybinding, 1)) == NULL) return(-1);
  if (0 != owl_keybinding_init(kb, keyseq, command, function_fn, desc)) {
    g_free(kb);
    return(-1);
  }
  /* see if another matching binding, and if so remove it.
   * otherwise just add this one. 
   */
  for (i = owl_list_get_size(&km->bindings)-1; i>=0; i--) {
    curkb = owl_list_get_element(&km->bindings, i);
    if (owl_keybinding_equal(curkb, kb)) {
      owl_list_remove_element(&km->bindings, i);
      owl_keybinding_delete(curkb);
    }
  }
  return owl_list_append_element(&km->bindings, kb);  

}

/* removes the binding associated with the keymap */
int owl_keymap_remove_binding(owl_keymap *km, const char *keyseq)
{
  owl_keybinding *kb, *curkb;
  int i;

  if ((kb = g_new(owl_keybinding, 1)) == NULL) return(-1);
  if (0 != owl_keybinding_make_keys(kb, keyseq)) {
    g_free(kb);
    return(-1);
  }

  for (i = owl_list_get_size(&km->bindings)-1; i >= 0; i--) {
    curkb = owl_list_get_element(&km->bindings, i);
    if (owl_keybinding_equal(curkb, kb)) {
      owl_list_remove_element(&km->bindings, i);
      owl_keybinding_delete(curkb);
      return(0);
    }
  }
  return(-2);
}


/* returns a summary line describing this keymap.  the caller must free. */
char *owl_keymap_summary(const owl_keymap *km)
{
  if (!km || !km->name || !km->desc) return NULL;
  return g_strdup_printf("%-15s - %s", km->name, km->desc);
}

/* Appends details about the keymap to fm */
void owl_keymap_get_details(const owl_keymap *km, owl_fmtext *fm, int recurse)
{
  owl_fmtext_append_bold(fm, "KEYMAP - ");
  owl_fmtext_append_bold(fm, km->name);
  owl_fmtext_append_normal(fm, "\n");
  if (km->desc) {
    owl_fmtext_append_normal(fm, OWL_TABSTR "Purpose:    ");
    owl_fmtext_append_normal(fm, km->desc);
    owl_fmtext_append_normal(fm, "\n");
  }
  if (km->parent) {
    owl_fmtext_append_normal(fm, OWL_TABSTR "Has parent: ");
    owl_fmtext_append_normal(fm, km->parent->name);
    owl_fmtext_append_normal(fm, "\n");
  }
    owl_fmtext_append_normal(fm, "\n");
  if (km->default_fn) {
    owl_fmtext_append_normal(fm, OWL_TABSTR 
     "Has a default keypress handler (default_fn).\n");
  }
  if (km->prealways_fn) {
    owl_fmtext_append_normal(fm, OWL_TABSTR
     "Executes a function (prealways_fn) on every keypress.\n");
  }
  if (km->postalways_fn) {
    owl_fmtext_append_normal(fm, OWL_TABSTR 
     "Executes a function (postalways_fn) after handling every keypress.\n");
  }

  owl_fmtext_append_bold(fm, "\nKey bindings:\n\n");  
  if (recurse) {
    _owl_keymap_format_with_parents(km, fm);
  } else {
    _owl_keymap_format_bindings(km, fm);
  }
}

static void _owl_keymap_format_with_parents(const owl_keymap *km, owl_fmtext *fm)
{
  while (km) {
    _owl_keymap_format_bindings(km, fm);
    km = km->parent;
    if (km) {
      owl_fmtext_append_bold(fm, "\nInherited from ");
      owl_fmtext_append_bold(fm, km->name);
      owl_fmtext_append_bold(fm, ":\n\n");
    }
  }
}

static void _owl_keymap_format_bindings(const owl_keymap *km, owl_fmtext *fm)
{
  int i, nbindings;
  const owl_keybinding *kb;
  
  nbindings = owl_list_get_size(&km->bindings);
  for (i=0; i<nbindings; i++) {
    char buff[100];
    const owl_cmd *cmd;
    const char *tmpdesc, *desc = "";

    kb = owl_list_get_element(&km->bindings, i);
    owl_keybinding_tostring(kb, buff, 100);
    owl_fmtext_append_normal(fm, OWL_TABSTR);
    owl_fmtext_append_normal(fm, buff);
    owl_fmtext_append_spaces(fm, 11-strlen(buff));
    owl_fmtext_append_normal(fm, " - ");
    if (kb->desc && *kb->desc) {
      desc = kb->desc;
    } else if ((cmd=owl_function_get_cmd(kb->command))
	       && (tmpdesc = owl_cmd_get_summary(cmd))) {
      desc = tmpdesc;
    }
    owl_fmtext_append_normal(fm, desc);
    if (kb->command && *(kb->command)) {
      owl_fmtext_append_normal(fm, "   [");
      owl_fmtext_append_normal(fm, kb->command);
      owl_fmtext_append_normal(fm, "]");
    } 
    owl_fmtext_append_normal(fm, "\n");
  }
}



/***********************************************************************/
/***************************** KEYHANDLER ******************************/
/***********************************************************************/

/* NOTE: keyhandler has private access to the internals of keymap */

int owl_keyhandler_init(owl_keyhandler *kh)
{
  if (0 != owl_dict_create(&kh->keymaps)) return(-1); 
  kh->active = NULL;
  owl_keyhandler_reset(kh);
  return(0);
}

/* adds a new keymap */
void owl_keyhandler_add_keymap(owl_keyhandler *kh, owl_keymap *km)
{
  owl_dict_insert_element(&kh->keymaps, km->name, km, NULL);
}

owl_keymap *owl_keyhandler_create_and_add_keymap(owl_keyhandler *kh, const char *name, const char *desc, void (*default_fn)(owl_input), void (*prealways_fn)(owl_input), void (*postalways_fn)(owl_input))
{
  owl_keymap *km;
  km = g_new(owl_keymap, 1);
  if (!km) return NULL;
  owl_keymap_init(km, name, desc, default_fn, prealways_fn, postalways_fn);
  owl_keyhandler_add_keymap(kh, km);
  return km;
}

/* resets state and clears out key stack */
void owl_keyhandler_reset(owl_keyhandler *kh)
{
  kh->in_esc = 0;
  memset(kh->kpstack, 0, (OWL_KEYMAP_MAXSTACK+1)*sizeof(int));
  kh->kpstackpos = -1;
}

owl_keymap *owl_keyhandler_get_keymap(const owl_keyhandler *kh, const char *mapname)
{
  return owl_dict_find_element(&kh->keymaps, mapname);
}

/* free the list with owl_cmddict_namelist_cleanup */
void owl_keyhandler_get_keymap_names(const owl_keyhandler *kh, owl_list *l)
{
  owl_dict_get_keys(&kh->keymaps, l);
}

void owl_keyhandler_keymap_namelist_cleanup(owl_list *l)
{
  owl_list_cleanup(l, g_free);
}



/* sets the active keymap, which will also reset any key state.
 * returns the new keymap, or NULL on failure. */
const owl_keymap *owl_keyhandler_activate(owl_keyhandler *kh, const char *mapname)
{
  const owl_keymap *km;
  if (kh->active && !strcmp(mapname, kh->active->name)) return(kh->active);
  km = owl_dict_find_element(&kh->keymaps, mapname);
  if (!km) return(NULL);
  owl_keyhandler_reset(kh);
  kh->active = km;
  return(km);
}

/* processes a keypress.  returns 0 if the keypress was handled,
 * 1 if not handled, -1 on error, and -2 if j==ERR. */
int owl_keyhandler_process(owl_keyhandler *kh, owl_input j)
{
  const owl_keymap     *km;
  const owl_keybinding *kb;
  int i, match;

  if (!kh->active) {
    owl_function_makemsg("No active keymap!!!");
    return(-1);
  }

  /* deal with ESC prefixing */
  if (!kh->in_esc && j.ch == 27) {
    kh->in_esc = 1;
    return(0);
  }
  if (kh->in_esc) {
    j.ch = OWL_META(j.ch);
    kh->in_esc = 0;
  }
  
  kh->kpstack[++(kh->kpstackpos)] = j.ch;
  if (kh->kpstackpos >= OWL_KEYMAP_MAXSTACK) {
    owl_keyhandler_reset(kh);
    owl_function_makemsg("Too many prefix keys pressed...");
    return(-1);
  }

  /* deal with the always_fn for the map and parents */
  for (km=kh->active; km; km=km->parent) {
    if (km->prealways_fn) {
      km->prealways_fn(j);
    }
  }

  /* search for a match.  goes through active keymap and then
   * through parents... TODO:  clean this up so we can pull
   * keyhandler and keymap apart.  */
  for (km=kh->active; km; km=km->parent) {
    for (i=owl_list_get_size(&km->bindings)-1; i>=0; i--) {
      kb = owl_list_get_element(&km->bindings, i);
      match = owl_keybinding_match(kb, kh);
      if (match == 1) {		/* subset match */

	/* owl_function_debugmsg("processkey: found subset match in %s", km->name); */
	return(0);
      } else if (match == 2) {	/* exact match */
	/* owl_function_debugmsg("processkey: found exact match in %s", km->name); */
	owl_keybinding_execute(kb, j.ch);
	owl_keyhandler_reset(kh);
	if (km->postalways_fn) {
	  km->postalways_fn(j);
	}
	return(0);
      }
    }
  }

  /* see if a default action exists for the active keymap */
  if (kh->active->default_fn && kh->kpstackpos<1) {
    /*owl_function_debugmsg("processkey: executing default_fn");*/

    kh->active->default_fn(j);
    owl_keyhandler_reset(kh);
    if (kh->active->postalways_fn) {
      kh->active->postalways_fn(j);
    }
    return(0);
  }

  owl_keyhandler_invalidkey(kh);
  /* unable to handle */
  return(1);  
}

void owl_keyhandler_invalidkey(owl_keyhandler *kh)
{
    char kbbuff[500];
    owl_keybinding_stack_tostring(kh->kpstack, kh->kpstackpos+1, kbbuff, 500);
    owl_function_makemsg("'%s' is not a valid key in this context.", kbbuff);
    owl_keyhandler_reset(kh);
}

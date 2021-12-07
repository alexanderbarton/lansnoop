# Conventions

 * Objects have a unique ID stored in their `id` field.  This ID will be unique among that type of object. (At teast.)
 * Objects refer to other objects via a `foo_id` field.
 * An object is "done" when its `fini` field is set. Once set, the object won't be updated again.  It's safe to delete it.
 * No object will refer to a "fini" object.

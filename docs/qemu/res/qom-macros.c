#define OBJECT(obj) ((Object *)(obj))

#define OBJECT_CLASS(class) ((ObjectClass *)(class))

#define OBJECT_CHECK(type, obj, name)                                          \
  ((type *)object_dynamic_cast_assert(OBJECT(obj), (name), __FILE__, __LINE__, \
                                      __func__))

#define OBJECT_CLASS_CHECK(class_type, class, name)                            \
  ((class_type *)object_class_dynamic_cast_assert(                             \
      OBJECT_CLASS(class), (name), __FILE__, __LINE__, __func__))

#define OBJECT_GET_CLASS(class, obj, name)                                     \
  OBJECT_CLASS_CHECK(class, object_get_class(OBJECT(obj)), name)

#define DECLARE_CLASS_CHECKERS(ClassType, OBJ_NAME, TYPENAME)                  \
  static inline G_GNUC_UNUSED ClassType *OBJ_NAME##_GET_CLASS(                 \
      const void *obj) {                                                       \
    return OBJECT_GET_CLASS(ClassType, obj, TYPENAME);                         \
  }                                                                            \
                                                                               \
  static inline G_GNUC_UNUSED ClassType *OBJ_NAME##_CLASS(const void *klass) { \
    return OBJECT_CLASS_CHECK(ClassType, klass, TYPENAME);                     \
  }

#define DECLARE_INSTANCE_CHECKER(InstanceType, OBJ_NAME, TYPENAME)             \
  static inline G_GNUC_UNUSED InstanceType *OBJ_NAME(const void *obj) {        \
    return OBJECT_CHECK(InstanceType, obj, TYPENAME);                          \
  }

#define DECLARE_OBJ_CHECKERS(InstanceType, ClassType, OBJ_NAME, TYPENAME)      \
  DECLARE_INSTANCE_CHECKER(InstanceType, OBJ_NAME, TYPENAME)                   \
                                                                               \
  DECLARE_CLASS_CHECKERS(ClassType, OBJ_NAME, TYPENAME)

#define OBJECT_DECLARE_TYPE(InstanceType, ClassType, MODULE_OBJ_NAME)          \
  typedef struct InstanceType InstanceType;                                    \
  typedef struct ClassType ClassType;                                          \
                                                                               \
  G_DEFINE_AUTOPTR_CLEANUP_FUNC(InstanceType, object_unref)                    \
                                                                               \
  DECLARE_OBJ_CHECKERS(InstanceType, ClassType, MODULE_OBJ_NAME,               \
                       TYPE_##MODULE_OBJ_NAME)

/**
 * OBJECT_DECLARE_SIMPLE_TYPE:
 * @InstanceType: instance struct name
 * @MODULE_OBJ_NAME: the object name in uppercase with underscore separators
 *
 * This does the same as OBJECT_DECLARE_TYPE(), but with no class struct
 * declared.
 *
 * This macro should be used unless the class struct needs to have
 * virtual methods declared.
 */
#define OBJECT_DECLARE_SIMPLE_TYPE(InstanceType, MODULE_OBJ_NAME)              \
  typedef struct InstanceType InstanceType;                                    \
                                                                               \
  G_DEFINE_AUTOPTR_CLEANUP_FUNC(InstanceType, object_unref)                    \
                                                                               \
  DECLARE_INSTANCE_CHECKER(InstanceType, MODULE_OBJ_NAME,                      \
                           TYPE_##MODULE_OBJ_NAME)

// This is a example
OBJECT_DECLARE_TYPE(X86CPU, X86CPUClass, X86_CPU)

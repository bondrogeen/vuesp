// types.ts
export type Validator = (value: any, values?: Record<string, any>) => true | string;
export type ValidationSchema = Record<string, Validator[]>;

export interface FieldState {
  value: any;
  errors: string[];
  touched: boolean;
  dirty: boolean;
}

export interface FormState {
  values: Record<string, any>;
  errors: Record<string, string[]>;
  touched: Record<string, boolean>;
  dirty: Record<string, boolean>;
}

export interface DefineFieldOptions {
  props?: (context: FieldContext) => Record<string, any>;
}

export interface FieldContext {
  errors: string[];
  value: any;
  touched: boolean;
  dirty: boolean;
}

export interface UseFormOptions {
  validationSchema: ValidationSchema;
  initialValues?: Record<string, any>;
}

export interface UseFormReturn {
  values: Record<string, any>;
  errors: Record<string, string[]>;
  defineField: <T = any>(name: string, options?: DefineFieldOptions) => [Ref<T>, ComputedRef<Record<string, any>>];
  handleSubmit: (callback: (values: Record<string, any>) => void | Promise<void>) => (event?: Event) => Promise<void>;
  resetForm: () => void;
  validate: () => boolean;
}

// useForm.ts
import type { Ref, ComputedRef } from 'vue';
import { ref, reactive, computed } from 'vue';

export function useForm(options: UseFormOptions): UseFormReturn {
  const { validationSchema, initialValues = {} } = options;

  // Состояние формы
  const values = reactive<Record<string, any>>({});
  const errors = reactive<Record<string, string[]>>({});
  const touched = reactive<Record<string, boolean>>({});
  const dirty = reactive<Record<string, boolean>>({});

  // Инициализация полей
  const initializeFields = () => {
    Object.keys(validationSchema).forEach((field) => {
      values[field] = initialValues[field] || '';
      errors[field] = [];
      touched[field] = false;
      dirty[field] = false;
    });
  };

  // Валидация поля
  const validateField = (fieldName: string, value: any): boolean => {
    const validators = validationSchema[fieldName] || [];
    const fieldErrors: string[] = [];

    for (const validator of validators) {
      const result = validator(value, values);
      if (result !== true) {
        fieldErrors.push(result);
      }
    }

    errors[fieldName] = fieldErrors;
    return fieldErrors.length === 0;
  };

  // Валидация всей формы
  const validateForm = (): boolean => {
    let isValid = true;

    Object.keys(validationSchema).forEach((field) => {
      const fieldValid = validateField(field, values[field]);
      if (!fieldValid) {
        isValid = false;
      }
    });

    return isValid;
  };

  // Определение поля
  const defineField = <T = any>(fieldName: string, options: DefineFieldOptions = {}): [Ref<T>, ComputedRef<Record<string, any>>] => {
    if (!validationSchema[fieldName]) {
      console.warn(`Поле "${fieldName}" не найдено в validationSchema`);
    }

    // Реактивная ссылка на значение
    const valueRef = computed<T>({
      get: () => values[fieldName],
      set: (newValue: T) => {
        values[fieldName] = newValue;
        dirty[fieldName] = true;
        validateField(fieldName, newValue);
      },
    });

    // Атрибуты для input элемента
    const attrs = computed<Record<string, any>>(() => {
      const fieldContext: FieldContext = {
        errors: errors[fieldName] || [],
        value: values[fieldName],
        touched: touched[fieldName] || false,
        dirty: dirty[fieldName] || false,
      };

      const baseAttrs = {
        value: values[fieldName],
        onInput: (event: Event) => {
          const target = event.target as HTMLInputElement;
          valueRef.value = target.value as T;
        },
        onBlur: () => {
          touched[fieldName] = true;
          validateField(fieldName, values[fieldName]);
        },
      };

      // Добавляем кастомные props если есть
      if (options.props) {
        const customProps = options.props(fieldContext);
        return { ...baseAttrs, ...customProps };
      }

      return baseAttrs;
    });

    return [valueRef as Ref<T>, attrs];
  };

  // Обработчик отправки формы
  const handleSubmit = (callback: (values: Record<string, any>) => void | Promise<void>) => {
    return async (event?: Event): Promise<void> => {
      if (event) {
        event.preventDefault();
      }

      // Помечаем все поля как touched при отправке
      Object.keys(validationSchema).forEach((field) => {
        touched[field] = true;
      });

      const isValid = validateForm();

      if (isValid) {
        await callback(values);
      }
    };
  };

  // Сброс формы
  const resetForm = (): void => {
    Object.keys(validationSchema).forEach((field) => {
      values[field] = initialValues[field] || '';
      errors[field] = [];
      touched[field] = false;
      dirty[field] = false;
    });
  };

  // Инициализируем форму
  initializeFields();

  return {
    values,
    errors,
    defineField,
    handleSubmit,
    resetForm,
    validate: validateForm,
  };
}

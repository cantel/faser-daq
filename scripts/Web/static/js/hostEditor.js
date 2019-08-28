JSONEditor.defaults.editors.hostEditor = JSONEditor.AbstractEditor.extend({
  setValue: function(value,initial) {
    value = this.typecast(value||'');

    // Sanitize value before setting it
    var sanitized = value;
    if(this.enum_values.indexOf(sanitized) < 0) {
      sanitized = this.enum_values[0];
    }

    if(this.value === sanitized) {
      return;
    }

    if(initial) this.is_dirty = false;
    else if(this.jsoneditor.options.show_errors === "change") this.is_dirty = true;

    this.input.value = this.enum_options[this.enum_values.indexOf(sanitized)];
    if(this.select2) {
      if(this.select2v4)
        this.select2.val(this.input.value).trigger("change");
      else
        this.select2.select2('val',this.input.value);
    }
    this.value = sanitized;
    this.onChange();
    this.change();
  },
  register: function() {
    this._super();
    if(!this.input) return;
    this.input.setAttribute('name',this.formname);
  },
  unregister: function() {
    this._super();
    if(!this.input) return;
    this.input.removeAttribute('name');
  },
  getNumColumns: function() {
    if(!this.enum_options) return 3;
    var longest_text = this.getTitle().length;
    for(var i=0; i<this.enum_options.length; i++) {
      longest_text = Math.max(longest_text,this.enum_options[i].length+4);
    }
    return Math.min(12,Math.max(longest_text/7,2));
  },
  typecast: function(value) {
    if(this.schema.type === "host") {
      return value;
    }
	
  },
  getValue: function() {
    if (!this.dependenciesFulfilled) {
      return undefined;
    }
    return this.typecast(this.value);
  },
  preBuild: function() {
    var self = this;
    this.input_type = 'select';
    this.enum_options = [];
    this.enum_values = [];
    this.enum_display = [];
    var i;

    var hostOptions ;

    $.ajax({url:"/config/hostOptions", async: false, success:function(data){
		hostOptions = data.hostOptions;
    }});

    // host
    if(this.schema.type === "host") {
      self.enum_display = this.schema.options && this.schema.options.enum_titles || hostOptions;
      self.enum_options = hostOptions;
      self.enum_values = hostOptions;

      if(!this.isRequired()){
        self.enum_display.unshift(' ');
        self.enum_options.unshift('undefined');
        self.enum_values.unshift(undefined);
      }

    }
    // Other, not supported
    else {
      throw "host editor requires the type host.";
    }
  },
  build: function() {
    var self = this;
    if(!this.options.compact) this.header = this.label = this.theme.getFormInputLabel(this.getTitle(), this.isRequired());
    if(this.schema.description) this.description = this.theme.getFormInputDescription(this.schema.description);
    if(this.options.infoText) this.infoButton = this.theme.getInfoButton(this.options.infoText);
    if(this.options.compact) this.container.classList.add('compact');

    this.input = this.theme.getSelectInput(this.enum_options);
    this.theme.setSelectOptions(this.input,this.enum_options,this.enum_display);

    if(this.schema.readOnly || this.schema.readonly) {
      this.always_disabled = true;
      this.input.disabled = true;
    }

    // Set custom attributes on input element. Parameter is array of protected keys. Empty array if none.
    this.setInputAttributes([]);

    this.input.addEventListener('change',function(e) {
      e.preventDefault();
      e.stopPropagation();
      self.onInputChange();
    });

    this.control = this.theme.getFormControl(this.label, this.input, this.description, this.infoButton);
    this.container.appendChild(this.control);

    this.value = this.enum_values[0];
  },
  onInputChange: function() {
    var val = this.typecast(this.input.value);

    var new_val;
    // Invalid option, use first option instead
    if(this.enum_values.indexOf(val) === -1) {
      new_val = this.enum_values[0];
    }
    else {
      new_val = this.enum_values[this.enum_values.indexOf(val)];
    }

    // If valid hasn't changed
    if(new_val === this.value) return;

    this.is_dirty = true;

    // Store new value and propogate change event
    this.value = new_val;
    this.onChange(true);
  },
  setupSelect2: function() {
    // If the Select2 library is loaded use it when we have lots of items
    if(window.jQuery && window.jQuery.fn && window.jQuery.fn.select2 && !(this.schema.options && this.schema.options.disable_select2) && (this.enum_options.length > 2 || (this.enum_options.length && this.enumSource))) {
      var options = $extend({},JSONEditor.plugins.select2);
      if(this.schema.options && this.schema.options.select2_options) options = $extend(options,this.schema.options.select2_options);
      this.select2 = window.jQuery(this.input).select2(options);
      this.select2v4 = this.select2.select2.hasOwnProperty("amd");

      var self = this;
      this.select2.on('select2-blur',function() {
        if(self.select2v4)
          self.input.value = self.select2.val();
        else
          self.input.value = self.select2.select2('val');

        self.onInputChange();
      });

      this.select2.on('change',function() {
        if(self.select2v4)
          self.input.value = self.select2.val();
        else
          self.input.value = self.select2.select2('val');

        self.onInputChange();
      });
    }
    else {
      this.select2 = null;
    }
  },
  postBuild: function() {
    this._super();
    this.theme.afterInputReady(this.input);
    this.setupSelect2();
  },
  enable: function() {
    if(!this.always_disabled) {
      this.input.disabled = false;
      if(this.select2) {
        if(this.select2v4)
          this.select2.prop("disabled",false);
        else
          this.select2.select2("enable",true);
      }
    }
    this._super();
  },
  disable: function(always_disabled) {
    if(always_disabled) this.always_disabled = true;
    this.input.disabled = true;
    if(this.select2) {
      if(this.select2v4)
        this.select2.prop("disabled",true);
      else
        this.select2.select2("enable",false);
    }
    this._super();
  },
  destroy: function() {
    if(this.label && this.label.parentNode) this.label.parentNode.removeChild(this.label);
    if(this.description && this.description.parentNode) this.description.parentNode.removeChild(this.description);
    if(this.input && this.input.parentNode) this.input.parentNode.removeChild(this.input);
    if(this.select2) {
      this.select2.select2('destroy');
      this.select2 = null;
    }

    this._super();
  },
  showValidationErrors: function (errors) {
    var self = this;

    if (this.jsoneditor.options.show_errors === "always") {}
    else if(!this.is_dirty && this.previous_error_setting===this.jsoneditor.options.show_errors) return;

    this.previous_error_setting = this.jsoneditor.options.show_errors;

    var messages = [];
    $.each(errors, function (i, error) {
      if (error.path === self.path) {
        messages.push(error.message);
      }
    });

    if (messages.length) {
      this.theme.addInputError(this.input, messages.join('. ') + '.');
    }
    else {
      this.theme.removeInputError(this.input);
    }
  }
});

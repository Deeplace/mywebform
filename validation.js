Drupal.behaviors.mywebform_validate	= {
  attach: function (context) {

    jQuery('#edit-save-draft').live('click', function(){
      jQuery('#mywebform-edit-form #document_state').val('draft');
      ret = true;

      if(jQuery('#DinamicDocuments').length > 0) {
        var wrapper = jQuery('#DinamicDocuments').parents('.multistep-container');
        if(wrapper.length > 0) {
          var ret = validateWebform(true, wrapper);
        }
        else {
          var ret = validateWebform(true, jQuery('#DinamicDocuments'));
        }
      }

      if(!ret) return false;
      jQuery('tr.drow').remove();
      submit_documents();
      if(document_set_title) {
        submit_document_user_title();
      }

    });

    jQuery('#mywebform-edit-form #edit-submit').live('click', function(){
      var ret = validateWebform(true, jQuery('#mywebform-edit-form'));

      if(!ret) return false;
      submit_documents();

      if(document_set_title) {
        submit_document_user_title();
      }
      jQuery('tr.drow').remove();
    });

    if(Drupal.settings.mywebform.revalidate) {
      webform.afterLoad.revalidate = function() {validateWebform(false, jQuery('#mywebform-edit-form')); };
    }

    jQuery('input').live('change', function() {
      switch(jQuery(this).attr('check_type')) {
        case 'custom':
          var val = jQuery(this).val();
          if(val.length) {
            jQuery(this).val(sprintf(jQuery(this).attr('format'), val));
          }
          break;
        case 'money':
          var val = jQuery(this).val();
          if(val.length) {
            val = formatNumber(val, 2);
            jQuery(this).val(val);
          }
          break;
        /*case 'numeric':
          var decimal_length = parseInt(jQuery(this).attr('decimal_length'));
          if(isNaN(decimal_length)) decimal_length = 0;
          var val = formatNumber(jQuery(this).val(), decimal_length);
          jQuery(this).val(val);
          break;*/
      }
    });

    function submit_document_user_title() {
      var did = jQuery('#documents-document-user-title input[name="did"]').val();
      var title = jQuery('#documents-document-user-title #edit-doc-title').val();
      var req = jQuery.ajax({
        'data': {'did': did, 'title': title},
        'async': false,
        'type': 'POST',
        'url': '/document/ajax/save_title',
        'dataType': 'json',
        'success': function(obj) {
           result = obj;
         }
      });
    }
  }
};

webformValidator.validators.validateForm = function (v, allowOverpass, validateArea) {
  var valid = true;
  jQuery('#mywebform-multistep li a', validateArea) .removeClass('exist-error');

  jQuery('tr.drow input, tr.drow select, tr.drow textarea', validateArea).addClass('skip-check');

  jQuery("select", validateArea).each(function (index) {
    if(!jQuery(this).hasClass('skip-check')) {
      jQuery(this).removeClass('error');
      if(jQuery(this).hasClass('required') && jQuery(this).val() == '') {
        var title = '';
        title = jQuery(this).attr('title');
        if(typeof title == 'undefined' || title == '') title = jQuery(this).attr('name');
        if(!jQuery(this).hasClass('error')) {
          jQuery(this).addClass('error');
          jQuery(this).parents('.selector').addClass('error');
        }
        v.errors.push(title + ' - ' + Drupal.t('This field is required'));
        valid = false;
        var id = jQuery(this).parents('.multistep-container').attr('id');
        jQuery('#mywebform-multistep li a[menuindex="' + id + '"]').addClass('exist-error');
      }
    }
  });

  jQuery("input", validateArea).each(function (index) {
    if(!jQuery(this).hasClass('skip-check')) {
      jQuery(this).removeClass('error');
      var value = jQuery(this).val();
      var title = '';
      var check_type = jQuery(this).attr('check_type');
      var minlength = parseInt(jQuery(this).attr("minlength"));
      var maxlength = parseInt(jQuery(this).attr("maxlength"));
      var decimal_length = parseInt(jQuery(this).attr("decimal_length"));
      var pattern;

      if(isNaN(minlength)) minlength = 0;
      if(isNaN(maxlength)) maxlength = 0;
      if(isNaN(decimal_length)) decimal_length = 0;

      title = jQuery(this).attr('title');
      if(typeof title == 'undefined' || title == '') title = jQuery(this).attr('name');

      // Check if value has data
      if(jQuery(this).hasClass('required') && value == '') {
        if(!jQuery(this).hasClass('error')) jQuery(this).addClass('error');
        v.errors.push(title + ' - ' + Drupal.t('This field is required'));
        valid = false;
        var id = jQuery(this).parents('.multistep-container').attr('id');
        jQuery('#mywebform-multistep li a[menuindex="' + id + '"]').addClass('exist-error');
      }

      if(minlength > 0 && value.length < minlength) {
        if(!jQuery(this).hasClass('error')) jQuery(this).addClass('error');
        v.errors.push(title + ' - ' + Drupal.t('This field must have at least @length symbols', {'@length' : minlength}));
        valid = false;
        var id = jQuery(this).parents('.multistep-container').attr('id');
        jQuery('#mywebform-multistep li a[menuindex="' + id + '"]').addClass('exist-error');
      }

      if(maxlength > 0 && value.length > maxlength) {
        if(!jQuery(this).hasClass('error')) jQuery(this).addClass('error');
        v.errors.push(title + ' - ' + Drupal.t('This field must have not more than @length symbols', {'@length' : maxlength}));
        valid = false;
        var id = jQuery(this).parents('.multistep-container').attr('id');
        jQuery('#mywebform-multistep li a[menuindex="' + id + '"]').addClass('exist-error');
      }

      if(value != '' && check_type) {
        var format_type = '';

        switch(check_type) {
          case 'date':
            pattern = new RegExp("^(([0-9]{2}[.]{1}[0-9]{2}[.]{1}[0-9]{4})|([0-9]{2}[/]{1}[0-9]{2}[/]{1}[0-9]{4}))$");
            format_type = Drupal.t('date');
            break;
          case 'period':
            var myregexp = /^((a|A)\/\d{4}$)|((t|T)\/[1234]\/\d{4}jQuery)|((s|S)\/[12]\/\d{4}$)|((l|L)\/\d{1,2}\/\d{4})$/;
            pattern = new RegExp(myregexp);
            format_type = Drupal.t('period');
            break;
          case 'latintext':
            pattern = new RegExp("^[A-Za-z0-9.,]*$");
            format_type = Drupal.t('latin text');
            break;
          case 'numeric':
            pattern = new RegExp("^([-][0-9]{1})?[0-9]*([.,,][0-9]+)?$");
            format_type = Drupal.t('number');
            break;
        }

        if (pattern) {
          if(!pattern.test(value)) {
            if(!jQuery(this).hasClass('error')) jQuery(this).addClass('error');
            v.errors.push(title + ' - ' + Drupal.t('Wrong field format: @type needed', {'@type' : format_type}));
            valid = false;
            var id = jQuery(this).parents('.multistep-container').attr('id');
            jQuery('#mywebform-multistep li a[menuindex="' + id + '"]').addClass('exist-error');
          }
        }
      }
    }
  });

  jQuery("textarea", validateArea).each(function (index) {
      if(!jQuery(this).hasClass('skip-check')) {
        jQuery(this).removeClass('error');
        var value = jQuery(this).val();
        var title = '';
        var check_type = jQuery(this).attr('check_type');
        var minlength = parseInt(jQuery(this).attr("minlength"));
        var maxlength = parseInt(jQuery(this).attr("maxlength"));

        var pattern;

        if(isNaN(minlength)) minlength = 0;
        if(isNaN(maxlength)) maxlength = 0;

        title = jQuery(this).attr('title');
        if(typeof title == 'undefined' || title == '') title = jQuery(this).attr('name');

        // Check if value has data
        if(jQuery(this).hasClass('required') && value == '') {
          if(!jQuery(this).hasClass('error')) jQuery(this).addClass('error');
          v.errors.push(title + ' - ' + Drupal.t('This field is required'));
          valid = false;
          var id = jQuery(this).parents('.multistep-container').attr('id');
          jQuery('#mywebform-multistep li a[menuindex="' + id + '"]').addClass('exist-error');
        }

        if(minlength > 0 && value.length < minlength) {
          if(!jQuery(this).hasClass('error')) jQuery(this).addClass('error');
          v.errors.push(title + ' - ' + Drupal.t('This field must have at least @length symbols', {'@length' : minlength}));
          valid = false;
          var id = jQuery(this).parents('.multistep-container').attr('id');
          jQuery('#mywebform-multistep li a[menuindex="' + id + '"]').addClass('exist-error');
        }

        if(maxlength > 0 && value.length > maxlength) {
          if(!jQuery(this).hasClass('error')) jQuery(this).addClass('error');
          v.errors.push(title + ' - ' + Drupal.t('This field must have not more than @length symbols', {'@length' : maxlength}));
          valid = false;
          var id = jQuery(this).parents('.multistep-container').attr('id');
          jQuery('#mywebform-multistep li a[menuindex="' + id + '"]').addClass('exist-error');
        }
      }
    });

  jQuery('tr.drow input, tr.drow select, tr.drow textarea', validateArea).removeClass('skip-check');

  return valid;
}

function show_error(has_errors) {
  var text = '';
  var error_val = '';

  jQuery.each(webformValidator.errors, function(item, msg) {
    text += '<li>' + msg + '</li>';
    error_val += msg + "\n";
  });

  jQuery('#has_errors').val(error_val);
  if(has_errors) {
    jQuery('#mywebform-error').html('<ul>' + text + '</ul>');
    jQuery('#mywebform-error').show();
    jQuery('html, body').animate({scrollTop:0}, 'slow');
  }
}

function validateWebform(allowOverpass, validateArea) {
  var has_errors = false;
  webformValidator.errors = [];
  jQuery.each(webformValidator.validators, function() {
    var ret = this(webformValidator, allowOverpass, validateArea);
    if(ret == false) has_errors = true;
  });

  show_error(has_errors);
  if(has_errors) {
    return false;
  }

  return true;
}

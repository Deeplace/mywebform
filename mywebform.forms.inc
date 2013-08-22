<?php

require_once 'mywebform.private.inc';

function mywebform_type_edit_form($form, $form_state, $webform = NULL, $add = TRUE) {

  if(!$add && empty($webform)) {
    drupal_not_found();
    exit();
  }

  drupal_add_js("
    (function ($) {
    Drupal.behaviors.mywebform = {
      attach: function() {

        $('#edit-html-use-template').bind('click', function() {
          if($(this).attr('checked')) {
            $('.form-item-html-template-view').hide();
          }
          else {
            $('.form-item-html-template-view').show();
          }
        });

        if($('#edit-html-use-template').attr('checked') == true ) {
          $('.form-item-html-template-view').hide();
        }
      }
    };
    })(jQuery);
  ", 'inline');

  if($add) {
    $webform = new stdClass();
    drupal_set_title(t('Add new webform'));
  } else {
    drupal_set_title(t('Edit webform @name', array('@name' => $webform->name)));
  }

  $form['#attributes'] = array('enctype' => 'multipart/form-data');
  $form['#webform'] = $webform;
  $form['#aux'] = array();

  $form['general'] = array(
    '#type' => 'fieldset',
    '#title' => t('General'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['general']['tech_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Technical name'),
    '#description' => t('Machine readable technical name. It is used to identify the webform by its name. Must be unique and lowercase.'),
    '#default_value' => isset($webform->tech_name) ? $webform->tech_name : '',
    '#required' => TRUE,
  );

  $form['general']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('A short name for the webform.'),
    '#default_value' => isset($webform->name) ? $webform->name : '',
    '#required' => TRUE,
  );

  $form['general']['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#description' => t('A long title for the webform.'),
    '#default_value' => isset($webform->title) ? $webform->title : '',
    '#maxlength' => 255,
    '#required' => TRUE,
  );

  $form['general']['tid'] = array(
    '#type' => 'select',
    '#title' => t('Category'),
    '#description' => t('A category, the webform belongs to.'),
    '#options' => _mywebform_term_list(variable_get('mywebform_vid', 0), t('- None -')),
    '#default_value' => isset($webform->tid) ? $webform->tid : '',
  );

  $form['general']['document_destination_tid'] = array(
    '#type' => 'select',
    '#title' => t('Document destination'),
    '#options' => _documents_term_list(variable_get('document_destination_vid', 0), t('- None -')),
    '#default_value' => isset($webform->document_destination_tid) ? $webform->document_destination_tid : '',
    '#required' => TRUE,
  );

  $form['general']['html_template'] = array(
    '#type' => 'textarea',
    '#title' => t('HTML template for edit'),
    '#rows' => 4,
    '#required' => TRUE,
    '#default_value' => isset($webform->html_template) ? $webform->html_template : '',
  );

  $form['general']['autofields'] = array(
    '#type' => 'textarea',
    '#title' => t('Autocalculate expressions'),
    '#rows' => 4,
    '#default_value' => isset($webform->autofields) ? $webform->autofields : '',
  );

  $form['general']['status'] = array(
    '#type' => 'select',
    '#title' => t('Status'),
    '#options' => array(
      '1' => t('Enabled'),
      '0' => t('Disabled'),
    ),
    '#default_value' => isset($webform->status) ? $webform->status : '',
    '#required' => TRUE,
  );

  $form['files'] = array(
    '#type' => 'fieldset',
    '#title' => t('Files'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['files']['js'] = _mywebform_file_item('js_file', t('JavaScript'), t('JavaScript file'), isset($webform->files['js'][0]) ? $webform->files['js'][0] : null);
  $form['files']['css'] = _mywebform_file_item('css_file', t('CSS'), t('CSS file'), isset($webform->files['css'][0]) ? $webform->files['css'][0] : null);
  $form['files']['xsd'] = _mywebform_file_item('xsd_file', t('XSD'), t('XSD file'), isset($webform->files['xsd'][0]) ? $webform->files['xsd'][0] : null);
  for($i = 1; $i <= variable_get('mywebform_attachments_amount', 3); $i++) {
    $form['files']['doc' . $i] = _mywebform_file_item('doc_file' . $i, t('Document'), t('Document file'), isset($webform->files['doc'][$i - 1]) ? $webform->files['doc'][$i - 1] : null);
  }

  $form['actions'] = array(
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );

  $form['actions']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  if(!$add) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
    );
  }

  $form['actions']['cancel'] = array(
    '#markup' => l(t('Cancel'), isset($webform->id) ? 'admin/structure/mywebform/' . $webform->id : 'admin/structure/webform'),
  );


  return $form;
}

function mywebform_type_edit_form_submit($form, &$form_state) {
  $obj = $form['#webform'];

  if($form_state['values']['op'] == t('Delete')) {
    drupal_goto('admin/structure/mywebform/' . $obj->id . '/delete');
  }

  // General object
  $obj->tid = $form_state['values']['tid'];
  $obj->tech_name = $form_state['values']['tech_name'];
  $obj->name = $form_state['values']['name'];
  $obj->title = $form_state['values']['title'];
  $obj->html_template = $form_state['values']['html_template'];
  $obj->autofields = $form_state['values']['autofields'];
  $obj->status = $form_state['values']['status'];
  $obj->document_destination_tid = $form_state['values']['document_destination_tid'];
  // Set aux values
  if(empty($obj->aux)) $obj->aux = array();
  if(is_array($form['#aux'])) {
    foreach($form['#aux'] as $item) {
      if(is_array($form_state['values'][$item])) {
        $obj->aux[$item] = implode(', ', array_filter($form_state['values'][$item]));
      } else {
        $obj->aux[$item] = $form_state['values'][$item];
      }
    }
  }

  $obj->files['js'][0] = _mywebform_get_uploaded_file($form_state, 'js_file', 'js', isset($obj->files['js'][0]) ? $obj->files['js'][0] : null);

  $obj->files['css'][0] = _mywebform_get_uploaded_file($form_state, 'css_file', 'css', isset($obj->files['css'][0]) ? $obj->files['css'][0] : null);
  $obj->files['xsd'][0] = _mywebform_get_uploaded_file($form_state, 'xsd_file', 'xsd', isset($obj->files['xsd'][0]) ? $obj->files['xsd'][0] : null);
  for($i = 1; $i <= variable_get('mywebform_attachments_amount', 3); $i++) {
    $obj->files['doc'][$i] = _mywebform_get_uploaded_file($form_state, 'doc_file' . $i, 'pdf doc', isset($obj->files['doc'][$i - 1]) ? $obj->files['doc'][$i - 1] : null);
  }

  if(mywebform_save($obj)) {
    t($obj->title); // Force introduce string in locales_sources table
    drupal_set_message(t('The webform <em>@name</em> has been saved', array('@name' => $obj->name)));
    watchdog('mywebform', 'Webform @name saved', array('@name' => $obj->name));
    $form_state['redirect'] = 'admin/structure/mywebform/' . $obj->id;
  } else {
    drupal_set_message(t('The webform <em>@name</em> save failed', array('@name' => $obj->name)), 'error');
    watchdog('mywebform', 'Webform @name save failed', array('@name' => $obj->name), WATCHDOG_ERROR);
    $form_state['redirect'] = 'admin/structure/mywebform';
  }
}

function mywebform_type_delete_form($form, $form_state, $webform = NULL) {
  $form = array();

  if(empty($webform)) {
    drupal_not_found();
    exit();
  }

  $form['#webform'] = $webform;
  $question = t('Do you really want to delete the webform <em>@name</em>?', array('@name' => $webform->name));
  return confirm_form($form, $question, 'admin/structure/mywebform/' . $webform->id . '/edit');
}

function mywebform_type_delete_form_submit($form, &$form_state) {
  $obj = $form['#webform'];

  // Remove attached files
  foreach($obj->files as $collection) {
    foreach($collection as $file) {
      if(!empty($file->filename)) {
        file_delete(mywebform_files_dir() . '/' . $file->filename);
      }
      if(!empty($file->id)) {
        db_query("DELETE FROM {mywebform_files} WHERE id = %d ", $file->id);
      }
    }
  }

  // Remove fields
//  db_query("DELETE FROM {mywebform_fields} WHERE mywebform_id = %d ", $obj->id);
  db_delete('mywebform_fields')
  ->condition('mywebform_id', $obj->id)
  ->execute();

  // Remove form
  //db_query("DELETE FROM {mywebforms} WHERE id = %d ", $obj->id);
  db_delete("mywebforms")
  ->condition('id', $obj->id)
  ->execute();

  // Throw a message
  drupal_set_message(t('The webform <em>@name</em> has been deleted', array('@name' => $obj->name)));
  watchdog('mywebform', 'Webform @name deleted', array('@name' => $obj->name));
  $form_state['redirect'] = 'admin/structure/mywebform';
}

function mywebform_field_edit_form($form, $form_state, $field = NULL, $webform = NULL, $add = TRUE) {
  $form = array();

  if($add && empty($webform)) {
    drupal_not_found();
    exit();
  }

  if(!$add && empty($field)) {
    drupal_not_found();
    exit();
  }

  if(!$add) {
    $form['#field'] = $field;
    drupal_set_title(t('Edit field @name', array('@name' => $field->name)));
  } else {
    $field = new stdClass();
    $form['#webform'] = $webform;
    drupal_set_title(t('Add new field for webform @name', array('@name' => $webform->name)));
  }

  drupal_add_js("
(function ($) {
Drupal.behaviors.mywebform = {
  attach: function() {
    $('#edit-type').change(function () {
      var val = $(this).val();
      $('#edit-format-wrapper').css('display', val == 'custom' ? 'block' : 'none');
      $('#edit-decimal-length-wrapper').css('display', val == 'float' ? 'block' : 'none');
    }).trigger('change');

    $('#edit-widget').change(function () {
      var val = $(this).val();
      $('#edit-rows-wrapper').css('display', val == 'textarea' ? 'block' : 'none');
      $('#edit-options-wrapper').css('display', (val == 'select' || val == 'container') ? 'block' : 'none');
      $('#edit-container-wrapper').css('display', (val == 'select') ? 'block' : 'none');
    }).trigger('change');
  }
};
})(jQuery);
  ", 'inline');

  $form['general'] = array(
    '#type' => 'fieldset',
    '#title' => t('General'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['general']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#required' => TRUE,
    '#default_value' => isset($field->name) ? $field->name : '',
  );

  $form['general']['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => isset($field->title) ? $field->title : '',
  );

  $form['general']['xpath'] = array(
    '#type' => 'textfield',
    '#title' => t('XPath'),
    '#default_value' => isset($field->xpath) ? $field->xpath : '',
  );

  $form['general']['type'] = array(
    '#type' => 'select',
    '#title' => t('Type'),
    '#options' => _mywebform_get_field_types(),
    '#required' => TRUE,
    '#default_value' => isset($field->type) ? $field->type : '',
  );

  $form['general']['format'] = array(
    '#type' => 'textfield',
    '#title' => t('Format'),
    '#default_value' => isset($field->format) ? $field->format : '',
  );

  $form['general']['widget'] = array(
    '#type' => 'select',
    '#title' => t('Widget'),
    '#options' => _mywebform_get_field_widgets(),
    '#required' => TRUE,
    '#default_value' => isset($field->widget) ? $field->widget : '',
  );

  $form['general']['grid_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Grid name'),
    '#description' => t("If this element is dynamically generated within a grid, write its grid name here"),
    '#default_value' => isset($field->grid_name) ? $field->grid_name : '',
  );

  $form['general']['weight'] = array(
    '#type' => 'weight',
    '#title' => t('Weight'),
    '#description' => t("Is used to order the nodes in the result XML file after submit the webform"),
    '#delta' => 100,
    '#default_value' => !empty($field->weight) ? $field->weight : 0,
  );

  $form['general']['status'] = array(
    '#type' => 'select',
    '#title' => t('Status'),
    '#options' => array(
      '1' => t('Enabled'),
      '0' => t('Disabled'),
    ),
    '#required' => TRUE,
    '#default_value' => isset($field->status) ? $field->status : '',
  );

  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['options']['container'] = array(
    '#type' => 'textfield',
    '#title' => t('Container'),
    '#description' => t("Container variable's name to store a single copy of values. Use it only for grid."),
    '#default_value' => isset($field->container) ? $field->container : '',
  );

  $form['options']['ignore_empty'] = array(
    '#type' => 'checkbox',
    '#title' => t('Do not include in XML if is empty'),
    '#default_value' => isset($field->ignore_empty) ? $field->ignore_empty : '',
  );

  $form['options']['export_result'] = array(
    '#type' => 'checkbox',
    '#title' => t('Export value to resultset'),
    '#default_value' => isset($field->export_result) ? $field->export_result : '',
  );

  $form['options']['force_def_val'] = array(
    '#type' => 'checkbox',
    '#title' => t('Force value from default'),
    '#default_value' => isset($field->force_def_val) ? $field->force_def_val : '',
  );

  $form['options']['class'] = array(
    '#type' => 'textfield',
    '#title' => t('Class'),
    '#default_value' => isset($field->class) ? $field->class : '',
  );

  $form['options']['default_value'] = array(
    '#type' => 'textfield',
    '#title' => t('Default value'),
    '#autocomplete_path' => 'mywebform/predefined/autocomplete',
    '#default_value' => isset($field->default_value) ? $field->default_value : '',
  );

  $form['options']['options'] = array(
    '#type' => 'textarea',
    '#title' => t('Options'),
    '#description' => t('SQL query or PHP code encapsulated in &lt;?php ?&gt;, or list of key | value separated by line breaks'),
    '#rows' => 4,
    '#default_value' => isset($field->options) ? $field->options : '',
  );

  $form['options']['attributes'] = array(
    '#type' => 'textarea',
    '#title' => t('Attributes'),
    '#description' => t('Additional HTML attributes for this element, list of key | value separated by line breaks'),
    '#rows' => 4,
    '#default_value' => isset($field->attributes) ? $field->attributes : '',
  );

  $form['options']['max_file_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum upload size per file'),
    '#default_value' => isset($field->max_file_size) ? $field->max_file_size : '',
    '#description' => t('Specify the size limit that applies to each file separately. Enter a value like "512" (bytes), "80K" (kilobytes) or "50M" (megabytes) in order to restrict the allowed file size. If you leave this empty the file sizes will be limited only by PHP\'s maximum post and file upload sizes (current limit <strong>%limit</strong>).', array('%limit' => format_size(file_upload_max_size()))),
  );

  $form['options']['upload_file_extensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Permitted upload file extensions'),
    '#default_value' => isset($field->upload_file_extensions) ? $field->upload_file_extensions : '',
    '#description' => t('Extensions a user can upload to this field. Separate extensions with a space and do not include the leading dot. Leaving this blank will allow users to upload a file with any extension. Example: png, gif, jpg, jpeg'),
  );

  $form['options']['rows'] = array(
    '#type' => 'textfield',
    '#title' => t('Rows'),
    '#default_value' => isset($field->rows) ? $field->rows : '',
  );

  $form['options']['help_url_type'] = array(
    '#type' => 'radios',
    '#title' => t('Help url type'),
    '#options' => array(
      HELP_URL_PATH_RELATIVE => t('Relative'),
      HELP_URL_PATH_ABSOLUTE => t('Absolute'),
    ),
    '#default_value' => isset($field->help_url_type) ? $field->help_url_type : '',
    '#required' => TRUE
  );

  $form['options']['help_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Help url link'),
    '#default_value' => isset($field->help_url) ? $field->help_url : '',
    '#description' => t('If url is absolute nedeed to begin with "http://"'),
  );

  $form['options']['help_url_text'] = array(
    '#type' => 'textfield',
    '#title' => t('Help url text'),
    '#default_value' => isset($field->help_url_text) ? $field->help_url_text : '',
  );

  $form['options']['disabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Disabled'),
    '#default_value' => isset($field->disabled) ? $field->disabled : '',
  );

  $form['options']['readonly'] = array(
    '#type' => 'checkbox',
    '#title' => t('Readonly'),
    '#default_value' => isset($field->readonly) ? $field->readonly : '',
  );

  $form['options']['required'] = array(
    '#type' => 'checkbox',
    '#title' => t('Required'),
    '#default_value' => isset($field->required) ? $field->required : '',
  );

  $form['options']['may_be_required'] = array(
    '#type' => 'checkbox',
    '#title' => t('May be Required'),
    '#default_value' => isset($field->may_be_required) ? $field->may_be_required : '',
  );

  $form['validate'] = array(
    '#type' => 'fieldset',
    '#title' => t('Validate'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['validate']['minlength'] = array(
    '#type' => 'textfield',
    '#title' => t('Minimum length'),
    '#default_value' => isset($field->minlength) ? $field->minlength : '',
  );

  $form['validate']['maxlength'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum length'),
    '#default_value' => isset($field->maxlength) ? $field->maxlength : '',
  );

  $form['validate']['decimal_length'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum length of decimal part'),
    '#default_value' => isset($field->decimal_length) ? $field->decimal_length : '',
  );

  $form['actions'] = array(
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );

  $form['actions']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  if(!$add) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
    );
  }

  $form['actions']['cancel'] = array(
    '#markup' => l(t('Cancel'), 'admin/structure/mywebform/' . (isset($field->mywebform_id) ? $field->mywebform_id : $webform->id)),
  );

  return $form;
}

function mywebform_field_edit_form_submit($form, &$form_state) {
  $obj = isset($form['#field']) ? $form['#field'] : new stdClass();

  if($form_state['values']['op'] == t('Delete')) {
    drupal_goto('admin/structure/mywebform/field/' . $obj->id . '/delete');
  }

  if(isset($form['#webform'])) {
    $obj->mywebform_id = $form['#webform']->id;
  }

  $obj->name = $form_state['values']['name'];
  $obj->title = $form_state['values']['title'];
  $obj->xpath = $form_state['values']['xpath'];
  $obj->grid_name = $form_state['values']['grid_name'];
  $obj->type = $form_state['values']['type'];
  $obj->widget = $form_state['values']['widget'];
  $obj->container = $form_state['values']['container'];
  $obj->format = $form_state['values']['format'];
  $obj->upload_file_extensions = $form_state['values']['upload_file_extensions'];
  $obj->max_file_size = $form_state['values']['max_file_size'];
  $obj->ignore_empty = $form_state['values']['ignore_empty'];
  $obj->export_result = $form_state['values']['export_result'];
  $obj->force_def_val = $form_state['values']['force_def_val'];
  $obj->disabled = $form_state['values']['disabled'];
  $obj->readonly = $form_state['values']['readonly'];
  $obj->required = $form_state['values']['required'];
  $obj->may_be_required = $form_state['values']['may_be_required'];
  $obj->weight = $form_state['values']['weight'];
  $obj->default_value = $form_state['values']['default_value'];
  $obj->options = $form_state['values']['options'];
  $obj->attributes = $form_state['values']['attributes'];
  $obj->class = $form_state['values']['class'];
  $obj->rows = $form_state['values']['rows'];
  $obj->help_url_type = $form_state['values']['help_url_type'];
  $obj->help_url = $form_state['values']['help_url'];
  $obj->help_url_text = $form_state['values']['help_url_text'];
  $obj->minlength = $form_state['values']['minlength'];
  $obj->maxlength = $form_state['values']['maxlength'];
  $obj->decimal_length = $form_state['values']['decimal_length'];
  $obj->status = $form_state['values']['status'];

  if(empty($obj->aux)) $obj->aux = array();
  $obj->aux = serialize($obj->aux);

  if(isset($obj->id)) {
    $result = drupal_write_record('mywebform_fields', $obj, 'id');
  } else {
    $obj->created = time();
    $result = drupal_write_record('mywebform_fields', $obj);
  }

  $webform = mywebform_load($obj->mywebform_id);
  if($result) {
    drupal_set_message(t('The field <em>@name</em> has been saved', array('@name' => $obj->name)));
    watchdog('mywebform', 'Field @name saved for webform @webform',
      array('@name' => $obj->name, '@webform' => $webform->tech_name));
  } else {
    drupal_set_message(t('The field <em>@name</em> save failed', array('@name' => $obj->name)), 'error');
    watchdog('mywebform', 'Field @name save failed for webform @webform',
      array('@name' => $obj->name, '@webform' => $webform->tech_name), WATCHDOG_ERROR);
  }

  $form_state['redirect'] = 'admin/structure/mywebform/' . $obj->mywebform_id;
}

function mywebform_field_delete_form($form, $form_state, $field = NULL) {
  $form = array();

  if(empty($field)) {
    drupal_not_found();
    exit();
  }

  $form['#field'] = $field;
  $question = t('Do you really want to delete the field <em>@name</em>?', array('@name' => $field->name));
  return confirm_form($form, $question, 'admin/structure/mywebform/field/' . $field->id . '/edit');
}

function mywebform_field_delete_form_submit($form, &$form_state) {
  $obj = $form['#field'];
  $webform = mywebform_load($obj->mywebform_id);

  //db_query("DELETE FROM {mywebform_fields} WHERE id = %d ", $obj->id);

  db_delete('mywebform_fields')
  ->condition('id', $obj->id)
  ->execute();

  drupal_set_message(t('The field <em>@name</em> has been deleted', array('@name' => $obj->name)));
  watchdog('mywebform', 'Field @name from webform @webform deleted', array('@name' => $obj->name, '@webform' => $webform->tech_name));
  $form_state['redirect'] = 'admin/structure/mywebform/' . $obj->mywebform_id;
}

function mywebform_import_fields_form($form, $form_state, $webform) {
  $form = array();

  if(empty($webform)) {
    drupal_not_found();
    exit();
  }

  $form['#attributes'] = array('enctype' => 'multipart/form-data');
  $form['#webform'] = $webform;

  $form['import_file'] = array(
    '#type' => 'file',
    '#title' => t('Import file'),
    '#description' => t('Allowed extensions: @ext', array('@ext' => 'xls')),
  );

  $form['import'] = array(
    '#type' => 'submit',
    '#value' => t('Import'),
  );

  return $form;
}

function mywebform_import_fields_form_submit($form, &$form_state) {
  set_time_limit(0);

  $webform = $form['#webform'];
  $file = file_save_upload('import_file', array('file_validate_extensions' => array('xls')));
  if(is_object($file)) {
    // Get XLS
    $rows = php_excel_load_file(drupal_realpath($file->uri));

    $header = array_shift($rows);
    // Remove empty columns from the end of the array
    while(1) {
      $value = array_pop($header);
      if(!is_null($value)) {
        $header[] = $value;
        break;
      }
    }

    // Parse table as fields
    $fields = array();
    foreach($rows as $row) {
      if(count(array_filter($row))) {
        if(count($row) == count($header)) {
          $fields[] = array_combine($header, $row);
        } else {
          $f = array();
          foreach($header as $index => $key) {
            $f[$key] = $row[$index];
          }
          $fields[] = $f;
        }
      }
    }

    // Get current fields ids
    $result = db_query("SELECT id, name FROM {mywebform_fields} WHERE mywebform_id = :mywebform_id ", array(':mywebform_id' => $webform->id));

    foreach($result as $data) {
      $field_ids[$data->name] = $data->id;
    }

    // Save fields
    foreach($fields as $field) {
      if(empty($field['name'])) continue;

      if(!$field['id'] && !empty($field_ids[$field['name']])) {
        $field['id'] = $field_ids[$field['name']];
      }
      $field['mywebform_id'] = $webform->id;

      if($field['id']) {
        if(drupal_write_record('mywebform_fields', $field, 'id')) {
          drupal_set_message(t('Field <em>@name</em> updated', array('@name' => $field['name'])));
        } else {
          drupal_set_message(t('Field <em>@name</em> update failed', array('@name' => $field['name'])), 'error');
        }
      } else {
        $field['created'] = time();
        if(drupal_write_record('mywebform_fields', $field)) {
          drupal_set_message(t('Field <em>@name</em> insert', array('@name' => $field['name'])));
        } else {
          drupal_set_message(t('Field <em>@name</em> insert failed', array('@name' => $field['name'])), 'error');
        }
      }
    }

    drupal_set_message(t('The fields have been imported successfuly'));
    return;
  }

  drupal_set_message(t('Failed to import fields'), 'error');
}

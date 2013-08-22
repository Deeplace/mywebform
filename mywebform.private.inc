<?php

/**
 * Inserts a complex file field in form
 *
 * @param string $name - name of the element
 * @param string $group_title - name of fieldset
 * @param string $item_title - name of the file field
 * @param object $file - the file's object from mywebform_files table
 * @return array - an associative array to by glued to $form array in form's function
 */
function _mywebform_file_item($name, $group_title, $item_title, $file) {
  $item = array();

  if(!empty($file)) {
    $item = array(
      '#type' => 'fieldset',
      '#title' => $group_title,
    );

    $file_obj = file_load($file->fid);

    $item['file_view'] = array(
      '#type' => 'item',
      '#title' => t('Uploaded file'),
      '#markup' => l($file_obj->filename, file_create_url($file_obj->uri), array('language' => language_default())),
      '#prefix' => '<div class="file-view">',
      '#suffix' => '</div>',
    );

    $item['remove_' . $name] = array(
      '#type' => 'checkbox',
      '#title' => t('Remove file'),
    );
  }

  $item[$name] = array(
    '#type' => 'file',
    '#title' => $item_title,
  );

  return $item;
}

/**
 * Uploads the file to module's file directory and returns its object
 *
 * @param array $form_state - drupal $form_state taken from form submit callback
 * @param string $form_item - field's name
 * @param string $extensions - the needed extensions separated by space to match file
 * @param object $obj - the original file's object from mywebform_files table
 * @return object - the updated file's object from mywebform_files table. If the
 *                  function fails, the return is NULL.
 */
function _mywebform_get_uploaded_file($form_state, $form_item, $extensions, $obj) {
  if(!is_object($obj)) $obj = new stdClass();

  if(!empty($form_state['values']['remove_'. $form_item]) && !empty($obj->fid)) {
    $old_file = file_load($obj->fid);
    $old_file->status = 0;
    file_save($old_file);
    $obj->fid = '';
  }

  global $conf;
  $conf['allow_insecure_uploads'] = 1;
  $file = file_save_upload($form_item, array('file_validate_extensions' => array($extensions)), 'public://' . mywebform_files_dir());

  if(is_object($file)) {
    $file->status = FILE_STATUS_PERMANENT;
    file_save($file);

    if(!empty($obj->fid)) {
      $old_file = file_load($obj->fid);
      $old_file->status = 0;
      file_save($old_file);
    }
    $obj->fid = $file->fid;
  }

  $conf['allow_insecure_uploads'] = 0;

  if(!empty($obj->fid) || !empty($obj->id)) {
    return $obj;
  }

  return NULL;
}

function _mywebform_get_field_types() {
  $types = array(
    'text' => 'text',
    'latintext' => 'latintext',
    'numeric' => 'numeric',
    'money' => 'money',
    'float' => 'float',
    'period' => 'period',
    'date' => 'date',
    'custom' => 'custom',
  );

  return $types;
}

function _mywebform_get_field_widgets() {
  $widgets = array(
    'textfield' => 'textfield',
    'textarea' => 'textarea',
    'checkbox' => 'checkbox',
    'select' => 'select',
    'hidden' => 'hidden',
    'file' => 'file',
    'container' => 'container',
  );

  return $widgets;
}

function _mywebform_term_list($vid, $empty_element = NULL) {
  $list = array();

  if(isset($empty_element)) {
    $list[''] = $empty_element;
  }

  $result = db_query("SELECT tid, name FROM {taxonomy_term_data} WHERE vid = :vid", array(':vid' => $vid));
  foreach($result as $data) {
    $list[$data->tid] = $data->name;
  }

  return $list;
}

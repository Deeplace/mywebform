<?php

function _mywebform_render_field_file($field, $value = '', $index, $preview) {
  $output = '';
  $attr = array();
  $name_index = '';
  $id_index = '';

  // Build name's and id's suffixes
  if(!empty($index)) {
    $name_index = '[' . implode('][', $index) . ']';
    $id_index = '-' . implode('-', $index);
  } elseif(!empty($field->grid_name)) {
    $name_index = '[0]';
  }

  $attr['name'] = $field->name;
  $attr['id'] = $field->name;
  $attr['class'] = array();

  if(!empty($field->help_url)) {
    $url_name = !empty($field->help_url_text) ? $field->help_url_text : $field->help_url;
    if($field->help_url_type == HELP_URL_PATH_RELATIVE) {
      $attr['help_url'] = l($url_name, $field->help_url, array('attributes' => array('target' => 'blank')));
    }
    else {
      $attr['help_url'] = l($url_name, $field->help_url, array('attributes' => array('target' => 'blank'), 'absolute' => TRUE));
    }
  }

  if($field->type) $attr['class'][] = $field->type;
  if($field->class) $attr['class'][] = $field->class;
  if($field->title) $attr['title'][] = $field->title;
  WebForm::SuplyAttributes($attr, $field->attributes);
  if($preview) {
    $attr['class'][] = 'form-item';
  } else {
    if(!empty($field->required)) $attr['class'][] = 'required';
    if(!empty($field->error)) $attr['class'][] = 'error';
  }

  // Get file data

  @list($md5, $fid, $url) = explode(';', $field->value, 3);
  if(!empty($fid)) {
    $file = file_load($fid);
    $arr = explode('.', $file->filename);
    $file->ext = strtolower(end($arr));
    $file->filelink = l($file->filename, 'getfile', array('query' => array('fid' => $file->fid)));
    $file->size = format_size($file->filesize);
  } else {
    $file = new stdClass();
  }

  if($preview) {
    if(!empty($file->fid)) {
      $output .= '<div class="file-info-wrapper">';
      $output .= '<div class="file-info">';
      $output .= '<div class="file ' . $file->ext . '">&nbsp;</div>';
      $output .= '<div class="filename">' . $file->filelink . '</div>';
      $output .= '<div class="filesize">' . $file->size . '</div>';
      $output .= '</div>';
      $output .= '</div>';
    } else {
      $attr['name'] .= $name_index;
      $attr['id'] .= $id_index;
      $output .= sprintf('<span %s>&nbsp;</span>', drupal_attributes($attr));
    }
  } else {
    $attr['disabled'] = $field->disabled ? 'disabled' : '';
    $attr = array_filter($attr);


    $output .= '<div class="file-info-wrapper">';
    $output .= '<div class="file-info"' . (empty($field->value) ? ' style="display: none;"' : '') . '>';
    $output .= '<div class="file ' . 			(empty($file->ext) ? '' : $file->ext ) . '">&nbsp;</div>';
    $output .= '<div class="filename">' . (empty($file->filelink) ? '' : $file->filelink ) . '</div>';
    $output .= '<div class="filesize">' . (empty($file->size) ? '' : $file->size ) . '</div>';
    $name = $attr['name'] . '-remove' . $name_index;
    $id = $attr['id'] . '-remove' . $id_index;
    $output .= '<div><input type="checkbox" name="' . $name . '" id="' . $id . '" />' . t('Remove') . '</div>';
    $output .= '</div>';

    $name = $attr['name'] . $name_index;
    $id = $attr['id'] . $id_index;
    $output .= '<input type="hidden" name="' . $name . '" id="' . $id . '" />';

    $attr['name'] = 'files[' . $attr['name'] . '-file]'; //. //$name_index;
    $attr['id'] .= '-file' . $id_index;
    $output .= '<input type="file" '. drupal_attributes($attr) . ' />';
    $output .= '</div>';
  }

  return $output;
}

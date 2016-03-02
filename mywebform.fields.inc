<?php

function _mywebform_render_field_file($field, $value, $index, $preview) {
  $output = '';
  $attr = array();
  $name_index = '';
  $id_index = '';

  // Build name's and id's suffixes
  if (!empty($index)) {
    $name_index = '[' . implode('][', $index) . ']';
    $id_index = '-' . implode('-', $index);
    $attr['row-index'] = $index;
  }
  elseif (!empty($field->grid_name)) {
    $name_index = '[0]';
    $attr['row-index'] = $index;
  }

  $attr['name'] = $field->name;
  $attr['id'] = $field->name;
  $attr['class'] = array();

  if ($field->type) {
    $attr['class'][] = $field->type;
  }

  if ($field->class) {
    $attr['class'][] = $field->class;
  }

  WebForm::SuplyAttributes($attr, $field->attributes);
  if ($preview) {
    $attr['class'][] = 'form-item';
  }
  else {
    if($field->required) $attr['class'][] = 'required';
    if($field->error) $attr['class'][] = 'error';
  }
  $attr['class'] = implode(' ', $attr['class']);

  // Get file data
  $fid = 0;
  if (!empty($value) && !is_array($value)) {
    list($md5, $fid, $url) = explode(';', $value, 3);
  }
  if (!empty($fid)) {
    $file = db_fetch_object(db_query("SELECT * FROM {files} WHERE fid = %d", $fid));
    $arr = explode('.', $file->filename);
    $file->ext = strtolower(end($arr));
    $file->filelink = l($file->filename, $file->filepath);
    $file->size = format_size($file->filesize);
  }
  else {
    $file = new stdClass();
  }

  if ($preview) {
    if (!empty($file->fid)) {
      $output .= '<div class="file-info">';
      $output .= '<div class="file ' . $file->ext . '">&nbsp;</div>';
      $output .= '<div class="filename">' . $file->filelink . '</div>';
      $output .= '<div class="filesize">' . $file->size . '</div>';
      $output .= '</div>';
    }
    elseif (!empty($field->grid_name)) {
      $attr['field'] = $field->name;

      $output .= sprintf("<span %s>", drupal_attributes($attr));
      $output .= '<div class="file">&nbsp;</div>';
      $output .= '<div class="filename"></div>';
      $output .= '<div class="filesize"></div>';
      $output .= '</span>';
    }
    else {
      $attr['field'] = $field->name;
      $attr['name'] .= $name_index;
      $attr['id'] .= $id_index;
      $output .= sprintf('<span %s>&nbsp;</span>', drupal_attributes($attr));
    }
  }
  else {
    $attr['disabled'] = $field->disabled ? 'disabled' : '';
    $attr = array_filter($attr);

    $output .= '<div>';
    $output .= '<div class="file-info"' . (empty($file->fid) ? ' style="display: none;"' : '') . '>';
    $output .= '<div class="file ' . $file->ext . '">&nbsp;</div>';
    $output .= '<div class="filename">' . $file->filelink . '</div>';
    $output .= '<div class="filesize">' . $file->size . '</div>';

    $a = array(
      'name' => $attr['name'] . '-remove' . $name_index,
      'id' => $attr['id'] . '-remove' . $id_index,
      'field' => $field->name . '-remove',
    );

    $output .= '<div><input type="checkbox" ' . drupal_attributes($a) . ' />' . t('Remove') . '</div>';
    $output .= '</div>';

    $a = array(
      'name' => $attr['name'] . $name_index,
      'id' => $attr['id'] . $id_index,
      'field' => $field->name,
      'value' => is_array($value) ? '' : $value,
    );
    $output .= '<input type="hidden" ' . drupal_attributes($a) . ' />';

    $attr['name'] .= '-file' . $name_index;
    $attr['id'] .= '-file' . $id_index;
    $attr['field'] = $field->name . '-file';

    $output .= '<input type="file" '. drupal_attributes($attr) . ' />';
    $output .= '</div>';
  }

  return $output;
}
